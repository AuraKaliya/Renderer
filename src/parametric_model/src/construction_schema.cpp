#include "renderer/parametric_model/construction_schema.h"

namespace renderer::parametric_model {
namespace {

ParametricConstructionInputTemplate nodeInput(ParametricInputSemantic semantic) {
    return {ParametricInputKind::node, semantic};
}

ParametricConstructionInputTemplate floatInput(ParametricInputSemantic semantic) {
    return {ParametricInputKind::float_value, semantic};
}

ParametricConstructionInputTemplate integerInput(ParametricInputSemantic semantic) {
    return {ParametricInputKind::integer_value, semantic};
}

ParametricConstructionInputTemplate vector3Input(ParametricInputSemantic semantic) {
    return {ParametricInputKind::vector3, semantic};
}

ParametricConstructionInputTemplate enumInput(ParametricInputSemantic semantic) {
    return {ParametricInputKind::enum_value, semantic};
}

}  // namespace

std::string_view ParametricConstructionSchema::constructionLabel(
    ParametricConstructionKind constructionKind)
{
    switch (constructionKind) {
    case ParametricConstructionKind::box_center_size:
        return "Box: center + size";
    case ParametricConstructionKind::box_center_corner_point:
        return "Box: center + corner point";
    case ParametricConstructionKind::box_corner_points:
        return "Box: two corner points";
    case ParametricConstructionKind::cylinder_center_radius_height:
        return "Cylinder: center + radius + height";
    case ParametricConstructionKind::cylinder_center_radius_point_height:
        return "Cylinder: center + radius point + height";
    case ParametricConstructionKind::cylinder_axis_endpoints_radius:
        return "Cylinder: axis endpoints + radius";
    case ParametricConstructionKind::sphere_center_radius:
        return "Sphere: center + radius";
    case ParametricConstructionKind::sphere_center_surface_point:
        return "Sphere: center + surface point";
    case ParametricConstructionKind::sphere_diameter_points:
        return "Sphere: diameter points";
    case ParametricConstructionKind::mirror_axis_plane:
        return "Mirror: axis + plane";
    case ParametricConstructionKind::linear_array_count_offset:
        return "Linear Array: count + offset";
    }

    return "Unknown";
}

std::string_view ParametricConstructionSchema::inputSemanticLabel(
    ParametricInputSemantic semantic)
{
    switch (semantic) {
    case ParametricInputSemantic::center:
        return "center";
    case ParametricInputSemantic::surface_point:
        return "surface point";
    case ParametricInputSemantic::corner_point:
        return "corner point";
    case ParametricInputSemantic::corner_start:
        return "corner start";
    case ParametricInputSemantic::corner_end:
        return "corner end";
    case ParametricInputSemantic::radius_point:
        return "radius point";
    case ParametricInputSemantic::axis_start:
        return "axis start";
    case ParametricInputSemantic::axis_end:
        return "axis end";
    case ParametricInputSemantic::diameter_start:
        return "diameter start";
    case ParametricInputSemantic::diameter_end:
        return "diameter end";
    case ParametricInputSemantic::width:
        return "width";
    case ParametricInputSemantic::height:
        return "height";
    case ParametricInputSemantic::depth:
        return "depth";
    case ParametricInputSemantic::radius:
        return "radius";
    case ParametricInputSemantic::slices:
        return "slices";
    case ParametricInputSemantic::stacks:
        return "stacks";
    case ParametricInputSemantic::segments:
        return "segments";
    case ParametricInputSemantic::axis:
        return "axis";
    case ParametricInputSemantic::plane_offset:
        return "plane offset";
    case ParametricInputSemantic::count:
        return "count";
    case ParametricInputSemantic::offset:
        return "offset";
    }

    return "unknown";
}

std::string_view ParametricConstructionSchema::inputSemanticOverlayLabel(
    ParametricInputSemantic semantic)
{
    switch (semantic) {
    case ParametricInputSemantic::surface_point:
        return "surface";
    case ParametricInputSemantic::corner_point:
        return "corner";
    case ParametricInputSemantic::plane_offset:
        return "plane";
    default:
        return inputSemanticLabel(semantic);
    }
}

std::vector<ParametricConstructionInputTemplate> ParametricConstructionSchema::inputTemplates(
    ParametricConstructionKind constructionKind)
{
    switch (constructionKind) {
    case ParametricConstructionKind::box_center_size:
        return {
            nodeInput(ParametricInputSemantic::center),
            floatInput(ParametricInputSemantic::width),
            floatInput(ParametricInputSemantic::height),
            floatInput(ParametricInputSemantic::depth)
        };
    case ParametricConstructionKind::box_center_corner_point:
        return {
            nodeInput(ParametricInputSemantic::center),
            nodeInput(ParametricInputSemantic::corner_point)
        };
    case ParametricConstructionKind::box_corner_points:
        return {
            nodeInput(ParametricInputSemantic::corner_start),
            nodeInput(ParametricInputSemantic::corner_end)
        };
    case ParametricConstructionKind::cylinder_center_radius_height:
        return {
            nodeInput(ParametricInputSemantic::center),
            floatInput(ParametricInputSemantic::radius),
            floatInput(ParametricInputSemantic::height),
            integerInput(ParametricInputSemantic::segments)
        };
    case ParametricConstructionKind::cylinder_center_radius_point_height:
        return {
            nodeInput(ParametricInputSemantic::center),
            nodeInput(ParametricInputSemantic::radius_point),
            floatInput(ParametricInputSemantic::height),
            integerInput(ParametricInputSemantic::segments)
        };
    case ParametricConstructionKind::cylinder_axis_endpoints_radius:
        return {
            nodeInput(ParametricInputSemantic::axis_start),
            nodeInput(ParametricInputSemantic::axis_end),
            floatInput(ParametricInputSemantic::radius),
            integerInput(ParametricInputSemantic::segments)
        };
    case ParametricConstructionKind::sphere_center_radius:
        return {
            nodeInput(ParametricInputSemantic::center),
            floatInput(ParametricInputSemantic::radius),
            integerInput(ParametricInputSemantic::slices),
            integerInput(ParametricInputSemantic::stacks)
        };
    case ParametricConstructionKind::sphere_center_surface_point:
        return {
            nodeInput(ParametricInputSemantic::center),
            nodeInput(ParametricInputSemantic::surface_point),
            integerInput(ParametricInputSemantic::slices),
            integerInput(ParametricInputSemantic::stacks)
        };
    case ParametricConstructionKind::sphere_diameter_points:
        return {
            nodeInput(ParametricInputSemantic::diameter_start),
            nodeInput(ParametricInputSemantic::diameter_end),
            integerInput(ParametricInputSemantic::slices),
            integerInput(ParametricInputSemantic::stacks)
        };
    case ParametricConstructionKind::mirror_axis_plane:
        return {
            enumInput(ParametricInputSemantic::axis),
            floatInput(ParametricInputSemantic::plane_offset)
        };
    case ParametricConstructionKind::linear_array_count_offset:
        return {
            integerInput(ParametricInputSemantic::count),
            vector3Input(ParametricInputSemantic::offset)
        };
    }

    return {};
}

std::optional<ParametricConstructionLinkTemplate> ParametricConstructionSchema::constructionLink(
    ParametricConstructionKind constructionKind)
{
    switch (constructionKind) {
    case ParametricConstructionKind::box_center_corner_point:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::center,
            ParametricInputSemantic::corner_point
        };
    case ParametricConstructionKind::box_corner_points:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::corner_start,
            ParametricInputSemantic::corner_end
        };
    case ParametricConstructionKind::cylinder_center_radius_point_height:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::center,
            ParametricInputSemantic::radius_point
        };
    case ParametricConstructionKind::cylinder_axis_endpoints_radius:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::axis_start,
            ParametricInputSemantic::axis_end
        };
    case ParametricConstructionKind::sphere_center_surface_point:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::center,
            ParametricInputSemantic::surface_point
        };
    case ParametricConstructionKind::sphere_diameter_points:
        return ParametricConstructionLinkTemplate {
            ParametricInputSemantic::diameter_start,
            ParametricInputSemantic::diameter_end
        };
    case ParametricConstructionKind::box_center_size:
    case ParametricConstructionKind::cylinder_center_radius_height:
    case ParametricConstructionKind::sphere_center_radius:
    case ParametricConstructionKind::mirror_axis_plane:
    case ParametricConstructionKind::linear_array_count_offset:
        return std::nullopt;
    }

    return std::nullopt;
}

}  // namespace renderer::parametric_model
