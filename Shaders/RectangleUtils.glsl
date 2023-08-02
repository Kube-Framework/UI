#include "FilledQuadUtils.glsl"
#include "PrimitiveCompute.glsl"

// Quad
struct Quad
{
    vec2 p1;
    vec2 p2;
    vec2 p3;
    vec2 p4;
};

layout (local_size_x = 1) in;

vec2[4] getUVs(const vec2 size, const uint spriteIndex, const uint fillMode)
{
    vec2[4] uvs = vec2[4](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    if (spriteIndex != NullSpriteIndex && fillMode == FillModeCrop) {
        // Query sprite size
        const vec2 spriteSize = textureSize(sprites[spriteIndex], 0);
        // Compute resize factor
        const bool isSpriteGreater = (spriteSize.x / spriteSize.y) >= (size.x / size.y);
        const float resizeFactor = float(!isSpriteGreater) * (size.x / spriteSize.x) + float(isSpriteGreater) * (size.y / spriteSize.y);
        // Compute scaling
        const vec2 newSize = vec2(spriteSize.x * resizeFactor, spriteSize.y * resizeFactor);
        const vec2 offset = vec2(
            (1.0 - (size.x / newSize.x)) / 2.0,
            (1.0 - (size.y / newSize.y)) / 2.0
        );
        uvs[0] += offset;
        uvs[1] += vec2(-offset.x, offset.y);
        uvs[2] -= offset;
        uvs[3] += vec2(offset.x, -offset.y);
    }
    return uvs;
}

Quad getQuad(const vec2 pos, const vec2 size, const uint spriteIndex, const uint fillMode, const vec2 rotationCosSin)
{
    Quad quad;
    vec2 transformedPos = pos;
    vec2 transformedSize = size;

    if (spriteIndex != NullSpriteIndex && fillMode == FillModeFit) {
        // Query sprite size
        const vec2 spriteSize = textureSize(sprites[spriteIndex], 0);
        // Compute resize factor
        const bool isSpriteGreater = (spriteSize.x / spriteSize.y) >= (size.x / size.y);
        const float resizeFactor = float(isSpriteGreater) * (size.x / spriteSize.x) + float(!isSpriteGreater) * (size.y / spriteSize.y);
        // Compute scaling
        transformedSize = vec2(spriteSize.x * resizeFactor, spriteSize.y * resizeFactor);
        transformedPos = vec2(
            pos.x + (size.x - transformedSize.x) / 2.0,
            pos.y + (size.y - transformedSize.y) / 2.0
        );
    }

    const mat2 rotationMatrix = mat2(rotationCosSin.x, -rotationCosSin.y, rotationCosSin.y, rotationCosSin.x);
    const vec2 rotationOrigin = transformedPos + transformedSize / 2.0;
    quad.p1 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformedPos));
    quad.p2 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformedPos + vec2(transformedSize.x, 0.0)));
    quad.p3 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformedPos + transformedSize));
    quad.p4 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformedPos + vec2(0.0, transformedSize.y)));
    return quad;
}