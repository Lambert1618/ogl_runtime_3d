#ifdef GL_OES_standard_derivatives
#  extension GL_OES_standard_derivatives : enable
#else
#  define use_fallback
#endif

varying highp vec2 sampleCoord;
varying highp vec2 shadowSampleCoord;
varying highp vec4 normalizedTextureBounds;

#ifdef use_fallback
varying highp vec2 alphas;
#endif

uniform sampler2D _qt_texture;
uniform highp vec4 color;
uniform highp vec4 shadowColor;

void main()
{
    highp float shadowDistance = texture2D(_qt_texture,
                                           clamp(shadowSampleCoord,
                                                 normalizedTextureBounds.xy,
                                                 normalizedTextureBounds.zw)).a;
#ifdef use_fallback
    highp float shadowAlpha = smoothstep(alphas.x, alphas.y, shadowDistance);
#else
    highp float shadowDistanceD = fwidth(shadowDistance);
    highp float shadowAlpha = smoothstep(0.5 - shadowDistanceD, 0.5, shadowDistance);
#endif

    highp vec4 shadowPixel = color * shadowColor * shadowAlpha;

    highp float textDistance = texture2D(_qt_texture,
                                         clamp(sampleCoord,
                                               normalizedTextureBounds.xy,
                                               normalizedTextureBounds.zw)).a;
#ifdef use_fallback
    highp float textAlpha = smoothstep(alphas.x, alphas.y, textDistance);
#else
    highp float textDistanceD = fwidth(textDistance);
    highp float textAlpha = smoothstep(0.5 - textDistanceD, 0.5, textDistance);
#endif

    highp vec4 textPixel = color * textAlpha;
    gl_FragColor = mix(shadowPixel, textPixel, textPixel.a);
}
