<Material name="carpaint_color_peel_2_layer" version="1.0">
    <MetaData >
        <Property formalName="Environment Map" name="uEnvironmentTexture" description="Environment texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="environment" default="./maps/materials/spherical_checker.png" category="Material"/>
        <Property formalName="Enable Environment" name="uEnvironmentMappingEnabled" description="Enable environment mapping" type="Boolean" default="True" category="Material"/>
        <Property formalName="Baked Shadow Map" name="uBakedShadowTexture" description="Baked shadow texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="shadow" default="./maps/materials/shadow.png" category="Material"/>
        <Property formalName="Shadow Mapping" name="uShadowMappingEnabled" description="Enable shadow mapping" type="Boolean" default="False" category="Material"/>
        <Property formalName="Gradient1D Map" description="Gradient texture of the material" hidden="True" name="randomGradient1D" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="gradient" default="./maps/materials/randomGradient1D.png"/>
        <Property formalName="Gradient2D Map" description="Gradient texture of the material" hidden="True" name="randomGradient2D" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="gradient" default="./maps/materials/randomGradient2D.png"/>
        <Property formalName="Gradient3D Map" description="Gradient texture of the material" hidden="True" name="randomGradient3D" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="gradient" default="./maps/materials/randomGradient3D.png"/>
        <Property formalName="Gradient4D Map" description="Gradient texture of the material" hidden="True" name="randomGradient4D" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="gradient" default="./maps/materials/randomGradient4D.png"/>
        <Property formalName="Coat Index of refraction" name="coating_ior" description="Index of refraction of the coat" type="Float" default="1.500000" category="Top coat"/>
        <Property formalName="Coat Weight" name="coat_weight" description="Weight of the coat" type="Float" default="1.000000" category="Top coat"/>
        <Property formalName="Coat Micro roughness" name="coat_roughness" description="Micro roughness of the coat" type="Float" default="0.000000" category="Top coat"/>
        <Property formalName="Coat Color" name="coat_color" description="Base color of the coat" type="Color" default="1 1 1" category="Top coat"/>
        <Property formalName="Flake Weight" name="flake_intensity" description="Intensity of the flakes" type="Float" default="0.500000" category="Flake layer"/>
        <Property formalName="Flake size" name="flake_size" description="Size of the flakes" type="Float" default="0.002000" category="Flake layer"/>
        <Property formalName="Flake amount" name="flake_amount" description="Amount of flakes" type="Float" default="0.220000" category="Flake layer"/>
        <Property formalName="Flake Micro roughness" name="flake_roughness" description="Micro roughness of the flakes" type="Float" default="0.200000" category="Flake layer"/>
        <Property formalName="Flake Color" name="flake_color" type="Color" description="Base color of the flakes" default="1 0.7 0.02" category="Flake layer"/>
        <Property formalName="Base Color" name="base_color" type="Color" description="Base color of the material" default="0.1 0.001 0.001" category="Base paint"/>
        <Property formalName="Flake Macro roughness" name="flake_bumpiness" description="Bumpiness of the flakes" type="Float" default="0.600000" category="Flake layer"/>
        <Property formalName="Orange Peel Size" name="peel_size" description="Orange peel bump size" type="Float" default="1.000000" category="Top coat"/>
        <Property formalName="Orange Peel Amount" name="peel_amount" description="Orange peel amount" type="Float" default="0.010000" category="Top coat"/>
    </MetaData>
    <Shaders type="GLSL" version="330">
    <Shader>
    <Shared>    </Shared>
<VertexShader>
        </VertexShader>
        <FragmentShader>

// add enum defines
#define texture_coordinate_uvw 0
#define texture_coordinate_world 1
#define texture_coordinate_object 2
#define scatter_reflect 0
#define scatter_transmit 1
#define scatter_reflect_transmit 2

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
texture_coordinate_info tmp2;
vec3 ftmp0;
vec3 ftmp1;
float ftmp2;
float ftmp3;
 vec4 tmpShadowTerm;

layer_result layers[3];

#include "SSAOCustomMaterial.glsllib"
#include "sampleLight.glsllib"
#include "sampleProbe.glsllib"
#include "sampleArea.glsllib"
#include "cube.glsllib"
#include "random255.glsllib"
#include "perlinNoise.glsllib"
#include "perlinNoiseBumpTexture.glsllib"
#include "coordinateSource.glsllib"
#include "square.glsllib"
#include "calculateRoughness.glsllib"
#include "evalBakedShadowMap.glsllib"
#include "evalEnvironmentMap.glsllib"
#include "luminance.glsllib"
#include "microfacetBSDF.glsllib"
#include "physGlossyBSDF.glsllib"
#include "simpleGlossyBSDF.glsllib"
#include "weightedLayer.glsllib"
#include "miNoise.glsllib"
#include "flakeNoiseBumpTexture.glsllib"
#include "flakeNoiseTexture.glsllib"
#include "diffuseReflectionBSDF.glsllib"
#include "fresnelLayer.glsllib"

bool evalTwoSided()
{
  return( false );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 0, 0, 0 ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].layer += tmpShadowTerm * microfacetBSDF( layers[0].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, coat_roughness, coat_roughness, scatter_reflect );

  layers[1].layer += tmpShadowTerm * microfacetBSDF( layers[1].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, flake_roughness, flake_roughness, scatter_reflect );

  layers[2].base += tmpShadowTerm * diffuseReflectionBSDF( normal, lightDir, lightDiffuse );
  layers[2].layer += tmpShadowTerm * microfacetBSDF( layers[2].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, 0.300000, 0.300000, scatter_reflect );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[0].tanFrame, varWorldPos, lightIdx, viewDir, coat_roughness, coat_roughness );

  layers[1].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[1].tanFrame, varWorldPos, lightIdx, viewDir, flake_roughness, flake_roughness );

  layers[2].base += tmpShadowTerm * lightDiffuse * sampleAreaDiffuse( layers[2].tanFrame, varWorldPos, lightIdx );
  layers[2].layer += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[2].tanFrame, varWorldPos, lightIdx, viewDir, 0.300000, 0.300000 );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QT3DS_ENABLE_LIGHT_PROBE
  layers[0].layer += tmpShadowTerm * microfacetSampledBSDF( layers[0].tanFrame, viewDir, coat_roughness, coat_roughness, scatter_reflect );

  layers[1].layer += tmpShadowTerm * microfacetSampledBSDF( layers[1].tanFrame, viewDir, flake_roughness, flake_roughness, scatter_reflect );

  layers[2].base += tmpShadowTerm * diffuseReflectionBSDFEnvironment( normal, 0.000000 ) * aoFactor;
  layers[2].layer += tmpShadowTerm * microfacetSampledBSDF( layers[2].tanFrame, viewDir, 0.300000, 0.300000, scatter_reflect );

#else
  layers[0].layer += tmpShadowTerm * sampleGlossy( layers[0].tanFrame, viewDir, coat_roughness );

  layers[1].layer += tmpShadowTerm * sampleGlossy( layers[1].tanFrame, viewDir, flake_roughness );

  layers[2].base += tmpShadowTerm * sampleDiffuse( layers[2].tanFrame ) * aoFactor;
  layers[2].layer += tmpShadowTerm * sampleGlossy( layers[2].tanFrame, viewDir, 0.300000 );

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
  return( false ? 1.0f : luminance( vec3( 1, 1, 1 ) ) );
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
     tmp2 = coordinateSource(texture_coordinate_object, 0 );
     ftmp0 = perlinNoiseBumpTexture( tmp2, peel_amount, peel_size, false, false, 0.000000, 1, false, vec3( 0.000000, 0.000000, 0.000000 ), 1.000000, 0.000000, 1.000000, normal );
     ftmp1 = flakeNoiseBumpTexture(tmp2, flake_size, flake_bumpiness, normal );
     ftmp2 = flakeNoiseTexture(tmp2, flake_intensity, flake_size, flake_amount ).mono;
     ftmp3 = flakeNoiseTexture(tmp2, 0.000000, 0.002200, 1.000000 ).mono;
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = weightedLayer( ftmp3, vec4( vec3( 1, 0.01, 0.01 ), 1.0).rgb, layers[2].layer, layers[2].base * vec4( base_color.rgb, 1.0), alpha );
  color = weightedLayer( ftmp2, flake_color.rgb, layers[1].layer, color, color.a );
  color = fresnelLayer( ftmp0, vec3( coating_ior ), coat_weight, coat_color.rgb, layers[0].layer, color, color.a );
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layers[0].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].tanFrame = orthoNormalize( mat3( tangent, cross(ftmp0, tangent), ftmp0 ) );
  layers[1].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[1].tanFrame = orthoNormalize( mat3( tangent, cross(ftmp1, tangent), ftmp1 ) );
  layers[2].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[2].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[2].tanFrame = orthoNormalize( mat3( tangent, cross(normal, tangent), normal ) );
}

        </FragmentShader>
    </Shader>
    </Shaders>
<Passes >
        <ShaderKey value="5"/>
        <LayerKey count="3"/>
    <Pass >
    </Pass>
</Passes>
</Material>
