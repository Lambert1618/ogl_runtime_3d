in vec2 sampleCoord;
in vec2 shadowSampleCoord;
in vec4 normalizedTextureBounds;

uniform sampler2D _qt_texture;
uniform vec4 color;
uniform vec4 shadowColor;

void main()
{
    float shadowDistance = texture(_qt_texture,
                                   clamp(shadowSampleCoord,
                                         normalizedTextureBounds.xy,
                                         normalizedTextureBounds.zw)).r;
    float shadowDistanceD = fwidth(shadowDistance);
    float shadowAlpha = smoothstep(0.5 - shadowDistanceD, 0.5, shadowDistance);
    vec4 shadowPixel = color * shadowColor * shadowAlpha;

    float textDistance = texture(_qt_texture,
                                 clamp(sampleCoord,
                                       normalizedTextureBounds.xy,
                                       normalizedTextureBounds.zw)).r;
    float textDistanceD = fwidth(textDistance);
    float textAlpha = smoothstep(0.5 - textDistanceD, 0.5, textDistance);
    vec4 textPixel = color * textAlpha;

    fragOutput = mix(shadowPixel, textPixel, textPixel.a);
}
