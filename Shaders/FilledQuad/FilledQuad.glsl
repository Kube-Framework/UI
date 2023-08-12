#include "../Primitive.glsl"

// Vertex
struct Vertex
{
    vec2 pos;
    vec2 center;
    vec2 halfSize;
    vec2 uv;
    vec4 radius;
    uint spriteIndex;
    uint color;
    uint borderColor;
    float borderWidth;
    float edgeSoftness;
    uint _padding;
    vec2 rotationOrigin;
    vec2 rotationCosSin;
};

// Quad
struct Quad
{
    vec2 p1;
    vec2 p2;
    vec2 p3;
    vec2 p4;
};

// Get the UV coordinates of a rectangle's sprite
Quad getRectangleUVQuad(const vec2 size, const uint spriteIndex, const uint fillMode)
{
    Quad uvs = Quad(
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    if (spriteIndex != NullSpriteIndex && fillMode == FillModeCrop) {
        // Query sprite size
        // const vec2 spriteSize = spriteSizes.sizes[spriteIndex];
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
        uvs.p1 += offset;
        uvs.p2 += vec2(-offset.x, offset.y);
        uvs.p3 -= offset;
        uvs.p4 += vec2(offset.x, -offset.y);
    }
    return uvs;
}

// Get the transformed quad of a rectangle in relative scale
Quad getRectangleRelativeQuad(const Area area, const uint spriteIndex, const uint fillMode, const vec2 rotationCosSin)
{
    Quad quad;
    Area transformed = area;

    if (spriteIndex != NullSpriteIndex && fillMode == FillModeFit) {
        // Query sprite size
        // const vec2 spriteSize = spriteSizes.sizes[spriteIndex];
        const vec2 spriteSize = textureSize(sprites[spriteIndex], 0);
        // Compute resize factor
        const bool isSpriteGreater = (spriteSize.x / spriteSize.y) >= (area.size.x / area.size.y);
        const float resizeFactor = float(isSpriteGreater) * (area.size.x / spriteSize.x) + float(!isSpriteGreater) * (area.size.y / spriteSize.y);
        // Compute scaling
        transformed.size = vec2(spriteSize.x * resizeFactor, spriteSize.y * resizeFactor);
        transformed.pos = vec2(
            area.pos.x + (area.size.x - transformed.size.x) / 2.0,
            area.pos.y + (area.size.y - transformed.size.y) / 2.0
        );
    }

    const mat2 rotationMatrix = getRotationMatrix(rotationCosSin);
    const vec2 rotationOrigin = transformed.pos + transformed.size / 2.0;
    quad.p1 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformed.pos));
    quad.p2 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformed.pos + vec2(transformed.size.x, 0.0)));
    quad.p3 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformed.pos + transformed.size));
    quad.p4 = toRelative(applyRotation(rotationMatrix, rotationOrigin, transformed.pos + vec2(0.0, transformed.size.y)));
    return quad;
}

Quad getTextUVQuad(const Area uvArea, const vec2 spriteSheetSize, const bool vertical)
{
    const Area ruvArea = Area(
        uvArea.pos / spriteSheetSize,
        uvArea.size / spriteSheetSize
    );

    const Quad tmp = Quad(
        ruvArea.pos,
        ruvArea.pos + vec2(ruvArea.size.x, 0.0),
        ruvArea.pos + ruvArea.size,
        ruvArea.pos + vec2(0.0, ruvArea.size.y)
    );

    return Quad(
        branchlessIf(vertical, tmp.p4, tmp.p1),
        branchlessIf(vertical, tmp.p1, tmp.p2),
        branchlessIf(vertical, tmp.p2, tmp.p3),
        branchlessIf(vertical, tmp.p3, tmp.p4)
    );
}

// Get the transformed quad of a text in relative scale
Quad getTextRelativeQuad(const Area area, const mat2 rotationMatrix, const vec2 rotationOrigin)
{
    return Quad(
        toRelative(applyRotation(rotationMatrix, rotationOrigin, area.pos)),
        toRelative(applyRotation(rotationMatrix, rotationOrigin, area.pos + vec2(area.size.x, 0.0))),
        toRelative(applyRotation(rotationMatrix, rotationOrigin, area.pos + area.size)),
        toRelative(applyRotation(rotationMatrix, rotationOrigin, area.pos + vec2(0.0, area.size.y)))
    );
}