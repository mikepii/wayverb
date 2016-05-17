#pragma once

#include "BottomPanel.hpp"
#include "ModelRendererComponent.hpp"
#include "ModelWrapper.hpp"
#include "RenderState.hpp"

class RightPanel final : public Component {
public:
    RightPanel(
        SceneData& scene_data,
        model::ValueWrapper<config::Combined>& combined_model,
        model::ValueWrapper<model::RenderStateManager>& render_state_manager);

    void resized() override;

private:
    ModelRendererComponent model_renderer_component;
};