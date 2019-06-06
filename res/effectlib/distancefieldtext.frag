#ifdef GL_OES_standard_derivatives
#  extension GL_OES_standard_derivatives : enable
#else
#  define use_fallback
#endif

varying highp vec2 sampleCoord;

uniform sampler2D _qt_texture;
uniform highp vec4 color;

#ifdef use_fallback
varying highp vec2 alphas;
#endif

void main()
{
    highp float distance = texture2D(_qt_texture, sampleCoord).a;

#ifdef use_fallback
    highp float alpha = smoothstep(alphas.x, alphas.y, distance);
#else
    highp float f = fwidth(distance);
    highp float alpha = smoothstep(0.5 - f, 0.5, distance);
#endif

    gl_FragColor = color * alpha;
}
