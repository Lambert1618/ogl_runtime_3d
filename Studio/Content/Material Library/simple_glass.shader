<Material name="simple_glass" version="1.0">
    <MetaData >
        <Property formalName="Environment Map" name="uEnvironmentTexture" description="Environment texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="environment" default="./maps/materials/spherical_checker.png" category="Material"/>
        <Property formalName="Enable Environment" name="uEnvironmentMappingEnabled" description="Enable environment mapping" type="Boolean" default="True" category="Material"/>
        <Property formalName="Baked Shadow Map" name="uBakedShadowTexture" description="Baked shadow texture for the material" type="Texture" filter="linear" minfilter="linearMipmapLinear" clamp="repeat" usage="shadow" default="./maps/materials/shadow.png" category="Material"/>
        <Property formalName="Shadow Mapping" name="uShadowMappingEnabled" description="Enable shadow mapping" type="Boolean" default="False" category="Material"/>
        <Property formalName="Fresnel Power" name="uFresnelPower" description="Fresnel power of the material" type="Float" default="1.0" category="Material"/>
        <Property formalName="Minimum Opacity" name="uMinOpacity" description="Minimum opacity of the material" type="Float" default="0.5" category="Material"/>
        <Property formalName="Reflectivity" name="reflectivity_amount" type="Float" min="0.000000" max="1.000000" default="1.000000" description="Reflectivity factor" category="Material"/>
        <Property formalName="Glass ior" name="glass_ior" hidden="True" type="Float" default="1.100000" description="Index of refraction of the material" category="Material"/>
        <Property formalName="Glass Color" name="glass_color" type="Color" default="0.9 0.9 0.9" description="Color of the material" category="Material"/>
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

#define QT3DS_ENABLE_UV0 1
#define QT3DS_ENABLE_WORLD_POSITION 1
#define QT3DS_ENABLE_TEXTAN 1
#define QT3DS_ENABLE_BINORMAL 0

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


// temporary declarations
vec3 ftmp0;
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
#include "abbeNumberIOR.glsllib"
#include "fresnelLayer.glsllib"
#include "refraction.glsllib"

bool evalTwoSided()
{
  return( true );
}

vec3 computeFrontMaterialEmissive()
{
  return( vec3( 0, 0, 0 ) );
}

void computeFrontLayerColor( in vec3 normal, in vec3 lightDir, in vec3 viewDir, in vec3 lightDiffuse, in vec3 lightSpecular, in float materialIOR, float aoFactor )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += tmpShadowTerm * microfacetBSDF( layers[0].tanFrame, lightDir, viewDir, lightSpecular, materialIOR, 0.000000, 0.000000, scatter_reflect_transmit );

#endif
}

void computeFrontAreaColor( in int lightIdx, in vec4 lightDiffuse, in vec4 lightSpecular )
{
#if QT3DS_ENABLE_CG_LIGHTING
  layers[0].base += tmpShadowTerm * lightSpecular * sampleAreaGlossy( layers[0].tanFrame, varWorldPos, lightIdx, viewDir, 0.000000, 0.000000 );

#endif
}

void computeFrontLayerEnvironment( in vec3 normal, in vec3 viewDir, float aoFactor )
{
#if !QT3DS_ENABLE_LIGHT_PROBE
  layers[0].base += tmpShadowTerm * microfacetSampledBSDF( layers[0].tanFrame, viewDir, 0.000000, 0.000000, scatter_reflect_transmit );

#else
  layers[0].base += tmpShadowTerm * sampleGlossy( layers[0].tanFrame, viewDir, 0.000000 );

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
  return( true ? 1.0 : luminance( vec3( abbeNumberIOR(glass_ior, 0.000000 ) ) ) );
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
     ftmp0 = vec3( reflectivity_amount );
     tmpShadowTerm = evalBakedShadowMap( texCoord0 );
}

vec4 computeLayerWeights( in float alpha )
{
  vec4 color;
  color = layers[0].base * vec4( ftmp0, 1.0);
  return color;
}


void initializeLayerVariables(void)
{
  // clear layers
  layers[0].base = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].layer = vec4(0.0, 0.0, 0.0, 1.0);
  layers[0].tanFrame = orthoNormalize( tangentFrame( normal, varWorldPos ) );
}

vec4 computeGlass(in vec3 normal, in float materialIOR, in float alpha, in vec4 color)
{
  vec4 rgba = color;
  float ratio = simpleFresnel( normal, materialIOR, uFresnelPower );
  vec3 absorb_color = ( log( glass_color.rgb )/-1.000000 );
  // prevent log(0) -> inf number issue
  if ( isinf(absorb_color.r) ) absorb_color.r = 1.0;
  if ( isinf(absorb_color.g) ) absorb_color.g = 1.0;
  if ( isinf(absorb_color.b) ) absorb_color.b = 1.0;
  rgba.rgb = mix(vec3(1.0) - absorb_color, rgba.rgb * (vec3(1.0) - absorb_color), ratio);
  rgba.a = mix(uMinOpacity, alpha, ratio);
  return rgba;
}

        </FragmentShader>
    </Shader>
    </Shaders>
<Passes >
        <ShaderKey value="36"/>
        <LayerKey count="1"/>
    <Pass >
        <Blending source="SrcAlpha" dest="OneMinusSrcAlpha"/>
        <RenderState name="CullFace"/>
    </Pass>
</Passes>
</Material>
