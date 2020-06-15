#include "main_window.h"
#include "Application.h"
#include "CommandIDs.h"
#include "try_and_explain.h"

#include "core/reverb_time.h"

#include "output/master.h"

//  init from as much outside info as possible
main_window::main_window(ApplicationCommandTarget& next,
                         String name,
                         std::string fname)
        : DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons)
        , next_command_target_{next}
        , model_{std::move(fname)}
        , content_component_{model_}
        , encountered_error_connection_{model_.connect_error_handler(
                  [this](auto err_str) {
                      AlertWindow::showMessageBoxAsync(
                              AlertWindow::AlertIconType::WarningIcon,
                              "render error",
                              err_str);
                  })}
        , begun_connection_{model_.connect_begun([this] {
            wayverb_application::get_command_manager().commandStatusChanged();
        })}
        , finished_connection_{model_.connect_finished([this] {
            wayverb_application::get_command_manager().commandStatusChanged();
        })} {
    content_component_.setSize(800, 600);
    setContentNonOwned(&content_component_, true);
    setUsingNativeTitleBar(true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    setResizable(true, false);
    setResizeLimits(400, 300, 100000, 100000);
    setVisible(true);

    auto& command_manager = wayverb_application::get_command_manager();
    command_manager.registerAllCommandsForTarget(this);
    addKeyListener(command_manager.getKeyMappings());

    const auto volume = wayverb::core::estimate_room_volume(
            model_.project.get_scene_data());
    model_.project.persistent.raytracer()->set_room_volume(volume);

    model_.reset_view();
    model_.scene.set_visualise(true);
}

main_window::~main_window() noexcept {
    removeKeyListener(
            wayverb_application::get_command_manager().getKeyMappings());
}

bool main_window::prepare_to_close() {
    if (model_.needs_save()) {
        switch (NativeMessageBox::showYesNoCancelBox(
                AlertWindow::AlertIconType::WarningIcon,
                "save?",
                "There are unsaved changes. Do you wish to save?")) {
            case 0:  // cancel
                return false;

            case 1:  // yes
                //  Attempt to save. Show a dialog if something goes wrong.
                try_and_explain([&] { save(); },
                                "saving project",
                                "Make sure the destination is writable.");

                //  If the model still needs saving for some reason (the user
                //  cancelled, an error ocurred), just return now.
                if (model_.needs_save()) {
                    return false;
                }

                //  Everything's fine, so carry on.
                break;

            case 2:  // no
                break;
        }
    }

    return true;
}

void main_window::closeButtonPressed() {
    if (prepare_to_close()) {
        wants_to_close_(*this);
    }
}

void main_window::getAllCommands(Array<CommandID>& commands) {
    commands.addArray({
            CommandIDs::idSaveProject,
            CommandIDs::idSaveAsProject,
            CommandIDs::idCloseProject,
            CommandIDs::idVisualise,
            CommandIDs::idResetView,
            CommandIDs::idStartRender,
            CommandIDs::idCancelRender,
    });
}

void main_window::getCommandInfo(CommandID command_id,
                                 ApplicationCommandInfo& result) {
    switch (command_id) {
        case CommandIDs::idSaveProject:
            result.setInfo("Save...", "Save", "General", 0);
            result.defaultKeypresses.add(
                    KeyPress('s', ModifierKeys::commandModifier, 0));
            break;

        case CommandIDs::idSaveAsProject:
            result.setInfo("Save As...", "Save as", "General", 0);
            result.defaultKeypresses.add(KeyPress(
                    's',
                    ModifierKeys::commandModifier | ModifierKeys::shiftModifier,
                    0));
            break;

        case CommandIDs::idCloseProject:
            result.setInfo("Close", "Close the current project", "General", 0);
            result.defaultKeypresses.add(
                    KeyPress('w', ModifierKeys::commandModifier, 0));
            break;

        case CommandIDs::idVisualise:
            result.setInfo("Visualise",
                           "Toggle display of ray and wave information",
                           "General",
                           0);
            result.setTicked(model_.scene.get_visualise());
            break;

        case CommandIDs::idResetView:
            result.setInfo("Reset View",
                           "Reset the 3D model view to its original position",
                           "General",
                           0);
            break;

        case CommandIDs::idStartRender:
            result.setInfo(
                    "Start Render", "Start rendering acoustics", "General", 0);
            result.setActive(!model_.is_rendering());
            break;

        case CommandIDs::idCancelRender:
            result.setInfo("Cancel Render",
                           "Cancel ongoing render operation",
                           "General",
                           0);
            result.setActive(model_.is_rendering());
            break;
    }
}

bool main_window::perform(const InvocationInfo& info) {
    switch (info.commandID) {
        case CommandIDs::idSaveProject:
            try_and_explain([&] { save(); }, "saving");
            return true;

        case CommandIDs::idSaveAsProject:
            try_and_explain([&] { save_as(); }, "saving as");
            return true;

        case CommandIDs::idCloseProject: closeButtonPressed(); return true;

        case CommandIDs::idVisualise:
            model_.scene.set_visualise(!model_.scene.get_visualise());
            return true;

        case CommandIDs::idResetView: model_.reset_view(); return true;

        case CommandIDs::idStartRender: {
            /// Modal callback 'checks out' the current output state
            output::get_output_options(model_.output, [this](auto ret) {
                if (ret != 0) {
                    const auto fnames =
                            wayverb::combined::model::compute_all_file_names(
                                    model_.project.persistent, model_.output);

                    for (const auto fname : fnames) {
                        if (File(fname).exists()) {
                            if (AlertWindow::showOkCancelBox(
                                        AlertWindow::AlertIconType::WarningIcon,
                                        "overwrite files?",
                                        "Existing files will be overwritten if "
                                        "you "
                                        "decide to continue with the rendering "
                                        "process.")) {
                                break;
                            } else {
                                return;
                            }
                        }
                    }

                    model_.start_render();
                }
            });
            return true;
        }

        case CommandIDs::idCancelRender: model_.cancel_render(); return true;

        default: return false;
    }
}

ApplicationCommandTarget* main_window::getNextCommandTarget() {
    return &next_command_target_;
}

void main_window::save() {
    model_.save([this] { return browse_for_file_to_save(); });
}

void main_window::save_as() {
    if (const auto fname = browse_for_file_to_save()) {
        model_.save_as(*fname);
    }
}

std::experimental::optional<std::string>
main_window::browse_for_file_to_save() {
    FileChooser fc{"save location...", File(), Project::project_wildcard};
    if (fc.browseForFileToSave(true)) {
        const auto path = fc.getResult();
        path.createDirectory();
        return path.getFullPathName().toStdString();
    }
    return std::experimental::nullopt;
}

main_window::wants_to_close::connection main_window::connect_wants_to_close(
        wants_to_close::callback_type callback) {
    return wants_to_close_.connect(std::move(callback));
}
