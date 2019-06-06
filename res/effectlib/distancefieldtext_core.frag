in vec2 sampleCoord;

uniform sampler2D _qt_texture;
uniform vec4 color;

void main()
{
    float distance = texture(_qt_texture, sampleCoord).r;
    float f = fwidth(distance);
    fragOutput = color * smoothstep(0.5 - f, 0.5, distance);
}
