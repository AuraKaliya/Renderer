#pragma once

#include <cmath>

#include "renderer/scene_contract/types.h"

namespace renderer::scene_contract::math {

constexpr float kPi = 3.14159265358979323846F;

inline Vec3f add(const Vec3f& lhs, const Vec3f& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline Vec3f subtract(const Vec3f& lhs, const Vec3f& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline Vec3f cross(const Vec3f& lhs, const Vec3f& rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

inline float dot(const Vec3f& lhs, const Vec3f& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float length(const Vec3f& value) {
    return std::sqrt(dot(value, value));
}

inline Vec3f normalize(const Vec3f& value) {
    const float valueLength = length(value);
    if (valueLength <= 0.0F) {
        return {0.0F, 0.0F, 0.0F};
    }

    const float inverseLength = 1.0F / valueLength;
    return {
        value.x * inverseLength,
        value.y * inverseLength,
        value.z * inverseLength
    };
}

inline Mat4f makeIdentity() {
    return {};
}

inline Mat4f makeTranslation(float x, float y, float z) {
    Mat4f matrix = makeIdentity();
    matrix.elements[12] = x;
    matrix.elements[13] = y;
    matrix.elements[14] = z;
    return matrix;
}

inline Mat4f makeScale(float x, float y, float z) {
    Mat4f matrix = {};
    matrix.elements[0] = x;
    matrix.elements[5] = y;
    matrix.elements[10] = z;
    matrix.elements[15] = 1.0F;
    return matrix;
}

inline Mat4f makeRotationX(float radians) {
    Mat4f matrix = makeIdentity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    matrix.elements[5] = cosine;
    matrix.elements[6] = sine;
    matrix.elements[9] = -sine;
    matrix.elements[10] = cosine;
    return matrix;
}

inline Mat4f makeRotationY(float radians) {
    Mat4f matrix = makeIdentity();
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    matrix.elements[0] = cosine;
    matrix.elements[2] = -sine;
    matrix.elements[8] = sine;
    matrix.elements[10] = cosine;
    return matrix;
}

inline Mat4f multiply(const Mat4f& lhs, const Mat4f& rhs) {
    Mat4f result = {};

    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0F;
            for (int index = 0; index < 4; ++index) {
                value += lhs.elements[index * 4 + row] * rhs.elements[column * 4 + index];
            }
            result.elements[column * 4 + row] = value;
        }
    }

    return result;
}

inline Mat4f makePerspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane) {
    Mat4f matrix = {};
    const float tangent = std::tan(verticalFovRadians * 0.5F);
    const float inverseDepth = 1.0F / (nearPlane - farPlane);

    matrix.elements[0] = 1.0F / (aspectRatio * tangent);
    matrix.elements[5] = 1.0F / tangent;
    matrix.elements[10] = (farPlane + nearPlane) * inverseDepth;
    matrix.elements[11] = -1.0F;
    matrix.elements[14] = (2.0F * farPlane * nearPlane) * inverseDepth;
    return matrix;
}

inline Mat4f makeLookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up) {
    const Vec3f forward = normalize(subtract(center, eye));
    const Vec3f side = normalize(cross(forward, up));
    const Vec3f upVector = cross(side, forward);

    Mat4f matrix = makeIdentity();
    matrix.elements[0] = side.x;
    matrix.elements[1] = upVector.x;
    matrix.elements[2] = -forward.x;

    matrix.elements[4] = side.y;
    matrix.elements[5] = upVector.y;
    matrix.elements[6] = -forward.y;

    matrix.elements[8] = side.z;
    matrix.elements[9] = upVector.z;
    matrix.elements[10] = -forward.z;

    matrix.elements[12] = -dot(side, eye);
    matrix.elements[13] = -dot(upVector, eye);
    matrix.elements[14] = dot(forward, eye);

    return matrix;
}

}  // namespace renderer::scene_contract::math
