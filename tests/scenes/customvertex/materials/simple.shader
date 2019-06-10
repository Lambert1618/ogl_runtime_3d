<Material name="simple" version="1.0">
    <MetaData>
        <Property formalName="Scale" name="scale" type="Vector" default="1 1 1" stage="vertex" category="Material" />
        <Property formalName="Color" name="color" type="Color" default="1 1 1" stage="fragment" category="Material" />
    </MetaData>
    <Shaders type="GLSL" version="330">
        <Shader>
            <VertexShader>
                attribute vec3 attr_pos;
                uniform mat4 modelViewProjection;

                void main() {
                    gl_Position = modelViewProjection * vec4(attr_pos * scale, 1.0);
                }
            </VertexShader>
            <FragmentShader>
                out vec4 fragColor;
                void main() {
                    fragColor = vec4(color.rgb, 1.0);
                }
            </FragmentShader>
        </Shader>
    </Shaders>
<Passes>
    <Pass>
    </Pass>
</Passes>
</Material>

