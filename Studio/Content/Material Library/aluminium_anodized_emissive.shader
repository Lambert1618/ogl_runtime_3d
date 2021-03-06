<Material name="aluminium_anodized_emissive" version="1.0">
    <MetaData >
        <Property formalName="Environment Map" name="uEnvironmentTexture" description="Environment texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="environment" default="./maps/materials/spherical_checker.png" category="Material"/>
        <Property formalName="Enable Environment" name="uEnvironmentMappingEnabled" description="Enable environment mapping" type="Boolean" default="True" category="Material"/>
        <Property formalName="Baked Shadow Map" name="uBakedShadowTexture" description="Baked shadow texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="shadow" default="./maps/materials/shadow.png" category="Material"/>
        <Property formalName="Shadow Mapping" name="uShadowMappingEnabled" description="Enable shadow mapping" type="Boolean" default="False" category="Material"/>
        <Property formalName="Roughness" name="roughness" type="Float" min="0.000000" max="1.000000" default="0.300000" description="Roughness of the material.\n0 = fully specular\n1 = fully diffuse" category="Material"/>
        <Property formalName="Base color" name="base_color" type="Color" default="0.7 0.7 0.7" description="Color of the material" category="Material"/>
        <Property formalName="Intensity" name="intensity" description="Emission intensity" type="Float" default="1.000000" category="Material"/>
        <Property formalName="Emission Color" name="emission_color" description="Color of the emission" type="Color" default="0 0 0" category="Material"/>
        <Property formalName="Emissive Map" name="emissive_texture" description="Emissive texture of the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="emissive" default="./maps/materials/emissive.png" category="Material"/>
        <Property formalName="Emissive Map Mask" name="emissive_mask_texture" description="Emissive mask texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="emissive_mask" default="./maps/materials/emissive_mask.png" category="Material"/>
    </MetaData>
    <Shaders type="GLSL" version="330">
    <Shader>
    <Shared>    </Shared>
<VertexShader>
        </VertexShader>
        <FragmentShader>

// add enum defines
#define scatter_reflect 0
#define scatter_transmit 1
#define scatter_reflect_transmit 2
#define mono_alpha 0
#define mono_average 1
#define mono_luminance 2
#define mono_maximum 3
#define wrap_clamp 0
#define wrap_repeat 1
#define wrap_mirrored_repeat 2
#define gamma_default 0
#define gamma_linear 1
#define gamma_srgb 2

#define QT3DS_ENABLE_UV0 1
#define QT3DS_ENABLE_WORLD_POSITION 1
#define QT3DS_ENABLE_TEXTAN 1
#define QT3DS_ENABLE_BINORMAL 1

#include "vertexFragmentBase.glsllib"

// set shader output
out vec4 fragColor;

// add structure defines
struct layer_result
{
  vec4 base;
  vec4 layer;
  mat3 tanFrame;
};


struct texture_coordinate_info
{
  vec3 position;
  vec3 tangent_u;
  vec3 tangent_v;
};


struct texture_return
{
  vec3 tint;
  float mono;
};


// temporary declarations
texture_coordinate_info tmp3;
 vec4 tmpShadowTerm;

layer_result layers[1];

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "square.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "luminance.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"
#include "fresnelLayer.glsllib"
#include "monoChannel.glsllib"
#include "fileTexture.glsllib"
#include "transformCoordinate.glsllib"
#include "rotationTranslationScale.glsllib"
#include "textureCoordinateInfo.glsllib"

bool evalTwoSided()
{
  return( false );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3(1.0) * vec3( vec3( ( intensity *( emission_color.rgb * ( fileTexture(emissive_texture, vec3(0.0), vec3(1.0), mono_alpha, tmp3, vec2(0.0, 1.0), vec2(0.0, 1.0), wrap_repeat, wrap_repeat, gamma_default ).tint * fileTexture(emissive_mask_texture, vec3(0.0), vec3(1.0), mono_alpha, tmp3, vec2(0.0, 1.0), vec2(0.0, 1.0), wrap_repeat, wrap_repeat, gamma_default ).tint ) ) ) ) ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += tmpShadowTerm * vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer += tmpShadowTerm * microfacetBSDF( layers[0].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, roughness, roughness, scatter_reflect );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[0].tanFrame, varWorldPos, lightIdx, viewDir, roughness, roughness );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QT3DS_ENABLE_LIGHT_PROBE
  layers[0].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += tmpShadowTerm * microfacetSampledBSDF( layers[0].tanFrame, viewDir, roughness, roughness, scatter_reflect );

#else
  layers[0].base += tmpShadowTerm * vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += tmpShadowTerm * sampleGlossy( layers[0].tanFrame, viewDir, roughness );

#endif
}

vec3 computeBackMaterialEmissive()
{
  return( vec3(0, 0, 0) );
}

void computeBackLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

void computeBackLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QT3DS_ENABLE_LIGHT_PROBE
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#else
  layers[0].base += vec4( 0.0, 0.0, 0.0, 1.0 );
  layers[0].layer += vec4( 0.0, 0.0, 0.0, 1.0 );
#endif
}

float computeIOR()
{
  return( false ? 1.0 : luminance( vec3( 1, 1, 1 ) ) );
}

float evalCutout()
{
  return( 1.000000 );
}

vec3 computeNormal()
{
  return( normal );
}

void computeTemporaries()
{
     tmp3 = transformCoordinate( rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, 1.000000 ) ), textureCoordinateInfo( texCoord0, tangent, binormal ) );
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = fresnelLayer( normal, vec3( 22, 22, 22 ), 1.000000, base_color.rgb, layers[0].layer, layers[0].base, alpha );
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layers[0].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].tanFrame = orthoNormalize( mat3( tangent, cross(normal, tangent), normal ) );
}

        </FragmentShader>
    </Shader>
    </Shaders>
<Passes >
        <ShaderKey value="4"/>
        <LayerKey count="1"/>
    <Pass >
    </Pass>
</Passes>
</Material>
