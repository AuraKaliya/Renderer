#pragma once

#include "renderer/parametric_model/evaluation_result.h"
#include "renderer/parametric_model/types.h"

namespace renderer::parametric_model {

class PrimitiveFactory {
public:
    [[nodiscard]] static ParametricNodeDescriptor makePointNode(
        const scene_contract::Vec3f& position);
    [[nodiscard]] static ParametricNodeDescriptor makeDirectionNode(
        const scene_contract::Vec3f& direction);
    [[nodiscard]] static ParametricNodeDescriptor makeAxisNode(
        const scene_contract::Vec3f& origin,
        const scene_contract::Vec3f& direction);
    [[nodiscard]] static ParametricNodeDescriptor makePlaneNode(
        const scene_contract::Vec3f& origin,
        const scene_contract::Vec3f& normal);
    [[nodiscard]] static ParametricNodeDescriptor makeScalarNode(float value);

    [[nodiscard]] static PrimitiveDescriptor makeBoxDescriptor(
        float width,
        float height,
        float depth);

    [[nodiscard]] static PrimitiveDescriptor makeCylinderDescriptor(
        float radius,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static PrimitiveDescriptor makeSphereDescriptor(
        float radius,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);

    [[nodiscard]] static FeatureDescriptor makePrimitiveFeature(
        const PrimitiveDescriptor& primitive);

    [[nodiscard]] static FeatureDescriptor makeMirrorFeature();

    [[nodiscard]] static FeatureDescriptor makeLinearArrayFeature();

    [[nodiscard]] static ParametricObjectDescriptor makeParametricObject(
        const PrimitiveDescriptor& basePrimitive);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricBoxFromCenterSize(
        const scene_contract::Vec3f& center,
        float width,
        float height,
        float depth);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricBoxFromCenterCornerPoint(
        const scene_contract::Vec3f& center,
        const scene_contract::Vec3f& cornerPoint);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricBoxFromCornerPoints(
        const scene_contract::Vec3f& cornerStart,
        const scene_contract::Vec3f& cornerEnd);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricCylinderFromCenterRadiusHeight(
        const scene_contract::Vec3f& center,
        float radius,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricCylinderFromCenterRadiusPointHeight(
        const scene_contract::Vec3f& center,
        const scene_contract::Vec3f& radiusPoint,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricCylinderFromAxisEndpointsRadius(
        const scene_contract::Vec3f& axisStart,
        const scene_contract::Vec3f& axisEnd,
        float radius,
        std::uint32_t segments = 24U);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricSphereFromCenterRadius(
        const scene_contract::Vec3f& center,
        float radius,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricSphereFromCenterSurfacePoint(
        const scene_contract::Vec3f& center,
        const scene_contract::Vec3f& surfacePoint,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);

    [[nodiscard]] static ParametricObjectDescriptor makeParametricSphereFromDiameterPoints(
        const scene_contract::Vec3f& diameterStart,
        const scene_contract::Vec3f& diameterEnd,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);

    [[nodiscard]] static EvaluationResult evaluate(const PrimitiveDescriptor& descriptor);
    [[nodiscard]] static EvaluationResult evaluate(const ParametricObjectDescriptor& descriptor);

    [[nodiscard]] static scene_contract::MeshData build(const PrimitiveDescriptor& descriptor);
    [[nodiscard]] static scene_contract::MeshData build(const ParametricObjectDescriptor& descriptor);

    [[nodiscard]] static scene_contract::MeshData makeBox(
        float width,
        float height,
        float depth);

    [[nodiscard]] static scene_contract::MeshData makeCylinder(
        float radius,
        float height,
        std::uint32_t segments = 24U);

    [[nodiscard]] static scene_contract::MeshData makeSphere(
        float radius,
        std::uint32_t slices = 24U,
        std::uint32_t stacks = 16U);
};

}  // namespace renderer::parametric_model
