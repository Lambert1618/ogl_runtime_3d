#ifdef GL_OES_standard_derivatives
#  extension GL_OES_standard_derivatives : enable
#else
#  define use_fallback
#endif

uniform highp mat4 mvp;
uniform highp float fontScale;
uniform int textureWidth;
uniform int textureHeight;
uniform highp vec2 shadowOffset;

attribute highp vec3 vCoord;
attribute highp vec2 tCoord;
attribute highp vec4 textureBounds;

varying highp vec2 sampleCoord;
varying highp vec2 shadowSampleCoord;
varying highp vec4 normalizedTextureBounds;

#ifdef use_fallback
varying highp vec2 alphas;

highp float thresholdFunc(highp float scale)
{
    highp float base = 0.5;
    highp float baseDev = 0.065;
    highp float devScaleMin = 0.15;
    highp float devScaleMax = 0.3;
    return base - ((clamp(scale, devScaleMin, devScaleMax) - devScaleMin)
                   / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

highp float spreadFunc(highp float scale)
{
    return 0.06 / scale;
}

highp vec2 alphaRange(highp float scale)
{
    highp float base = thresholdFunc(scale);
    highp float range = spreadFunc(scale);
    highp float alphaMin = max(0.0, base - range);
    highp float alphaMax = min(base + range, 1.0);
    return vec2(alphaMin, alphaMax);
}

highp float determinantOfSubmatrix(highp mat4 m, int col0, int col1, int row0, int row1)
{
    return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
}

highp float determinantOfSubmatrix(highp mat4 m, int col0, int col1, int col2,
                                   int row0, int row1, int row2)
{
    highp float det = m[col0][row0] * determinantOfSubmatrix(m, col1, col2, row1, row2);
    det            -= m[col1][row0] * determinantOfSubmatrix(m, col0, col2, row1, row2);
    det            += m[col2][row0] * determinantOfSubmatrix(m, col0, col1, row1, row2);
    return det;
}

highp float determinant(highp mat4 m)
{
    highp float det = m[0][0] * determinantOfSubmatrix(m, 1, 2, 3, 1, 2, 3);
    det            -= m[1][0] * determinantOfSubmatrix(m, 0, 2, 3, 1, 2, 3);
    det            += m[2][0] * determinantOfSubmatrix(m, 0, 1, 3, 1, 2, 3);
    det            -= m[3][0] * determinantOfSubmatrix(m, 0, 1, 2, 1, 2, 3);
    return det;
}
#endif

void main()
{
#ifdef use_fallback
    highp float scale = fontScale * pow(abs(determinant(mvp)), 1.0 / 3.0);
    alphas = alphaRange(scale);
#endif

    highp vec2 textureSizeMultiplier = vec2(1.0 / float(textureWidth),
                                            1.0 / float(textureHeight));

    sampleCoord = tCoord * textureSizeMultiplier;
    shadowSampleCoord = (tCoord - shadowOffset) * textureSizeMultiplier;
    normalizedTextureBounds = vec4(textureBounds.xy * textureSizeMultiplier,
                                   textureBounds.zw * textureSizeMultiplier);

    gl_Position = mvp * vec4(vCoord, 1.0);
}
