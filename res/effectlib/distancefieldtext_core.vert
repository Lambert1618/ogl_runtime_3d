in vec3 vCoord;
in vec2 tCoord;

out vec2 sampleCoord;

uniform mat4 mvp;
uniform int textureWidth;
uniform int textureHeight;
uniform float fontScale;

void main()
{
    sampleCoord = tCoord * vec2(1.0 / float(textureWidth), 1.0 / float(textureHeight));
    gl_Position = mvp * vec4(vCoord, 1.0);
}
