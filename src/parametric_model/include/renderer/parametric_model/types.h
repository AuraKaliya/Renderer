#pragma once

#include <cstdint>
#include <vector>

#include "renderer/scene_contract/types.h"

namespace renderer::parametric_model {

enum class PrimitiveKind : std::uint8_t {
    box,
    cylinder,
    sphere
};

using ParametricNodeId = std::uint32_t;
using ParametricUnitId = std::uint32_t;
using ParametricObjectId = std::uint32_t;
using ParametricFeatureId = std::uint32_t;

struct NodeReference {
    ParametricNodeId id = 0U;
};

enum class ParametricNodeKind : std::uint8_t {
    point
};

struct PointNodeDescriptor {
    scene_contract::Vec3f position {};
};

struct ParametricNodeDescriptor {
    ParametricNodeId id = 0U;
    ParametricNodeKind kind = ParametricNodeKind::point;
    PointNodeDescriptor point {};
};

struct BoxSpec {
    float width = 1.0F;
    float height = 1.0F;
    float depth = 1.0F;
};

struct CylinderSpec {
    float radius = 0.5F;
    float height = 1.0F;
    std::uint32_t segments = 24U;
};

struct SphereSpec {
    float radius = 0.5F;
    std::uint32_t slices = 24U;
    std::uint32_t stacks = 16U;
    enum class ConstructionMode : std::uint8_t {
        center_radius,
        center_surface_point
    };
    ConstructionMode constructionMode = ConstructionMode::center_radius;
    NodeReference center {};
    NodeReference surfacePoint {};
};

struct PrimitiveDescriptor {
    PrimitiveKind kind = PrimitiveKind::box;
    BoxSpec box {};
    CylinderSpec cylinder {};
    SphereSpec sphere {};
};

enum class Axis : std::uint8_t {
    x,
    y,
    z
};

struct MirrorOperatorSpec {
    Axis axis = Axis::x;
    float planeOffset = 0.0F;
};

struct LinearArrayOperatorSpec {
    std::uint32_t count = 1U;
    scene_contract::Vec3f offset {1.0F, 0.0F, 0.0F};
};

enum class FeatureKind : std::uint8_t {
    primitive,
    mirror,
    linear_array
};

enum class ParametricUnitKind : std::uint8_t {
    primitive_generator,
    mirror_operator,
    linear_array_operator
};

enum class ParametricUnitRole : std::uint8_t {
    generator,
    operator_unit
};

struct ParametricObjectMetadata {
    ParametricObjectId id = 0U;
    PrimitiveKind objectKind = PrimitiveKind::box;
    scene_contract::Vec3f pivot {0.0F, 0.0F, 0.0F};
};

struct FeatureDescriptor {
    ParametricFeatureId id = 0U;
    ParametricUnitId unitId = 0U;
    FeatureKind kind = FeatureKind::primitive;
    bool enabled = true;
    PrimitiveDescriptor primitive {};
    MirrorOperatorSpec mirror {};
    LinearArrayOperatorSpec linearArray {};
};

struct ParametricObjectDescriptor {
    ParametricObjectMetadata metadata {};
    std::vector<FeatureDescriptor> features;
    std::vector<ParametricNodeDescriptor> nodes;
};

struct ParametricUnitDescriptor {
    ParametricUnitId id = 0U;
    ParametricFeatureId featureId = 0U;
    ParametricUnitKind kind = ParametricUnitKind::primitive_generator;
    ParametricUnitRole role = ParametricUnitRole::generator;
    bool enabled = true;
};

}  // namespace renderer::parametric_model
