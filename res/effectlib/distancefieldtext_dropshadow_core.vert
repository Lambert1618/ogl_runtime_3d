in vec3 vCoord;
in vec2 tCoord;
in vec4 textureBounds;

out vec2 sampleCoord;
out vec2 shadowSampleCoord;

out vec4 normalizedTextureBounds;

uniform mat4 mvp;
uniform int textureWidth;
uniform int textureHeight;
uniform float fontScale;
uniform vec2 shadowOffset;

void main()
{
     vec2 textureSizeMultiplier = vec2(1.0 / float(textureWidth), 1.0 / float(textureHeight));

     sampleCoord = tCoord * textureSizeMultiplier;
     shadowSampleCoord = (tCoord - shadowOffset) * textureSizeMultiplier;
     normalizedTextureBounds = vec4(textureBounds.xy * textureSizeMultiplier,
                                    textureBounds.zw * textureSizeMultiplier);
     gl_Position = mvp * vec4(vCoord, 1.0);
}
