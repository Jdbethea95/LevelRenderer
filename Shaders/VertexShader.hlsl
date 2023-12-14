// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major) 
// TODO: Part 1F changed float2 to float3, changed return to a float 4 with w set to 1
// TODO: Part 1H 
// TODO: Part 2B
struct OBJ_ATTRIBUTES
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model

};

struct OutputToRasterizer
{
    
    float4 posH : SV_POSITION;
    float3 posW : WORLD;  //is this just the POSITION?
    float3 normW : NORMAL;
    float3 uvwW : UVW;
    uint instID : SV_InstanceID;
    uint viewport : SV_ViewportArrayIndex;
};

cbuffer SceneData : register(b0)
{
    float4 vec_SunDirection, vec_SunColor;
    float4x4 mat_View, mat_Projection;
    float4 sunAmbient, cameraPos;
};

cbuffer MeshData : register(b1)
{
    float4x4 mat_World[200];
    OBJ_ATTRIBUTES material[200];
};

cbuffer IdData : register(b2)
{
    float4 tmIDS;
};


// TODO: Part 2D 
// TODO: Part 4A 
// TODO: Part 4B                                       Do these need to still exist?
OutputToRasterizer main(float3 inputVertex : POSITION, float3 uvw : UVW, float3 nrm : NORMAL, uint insID : SV_InstanceID)
{
    
    //temp shift in position.
    //inputVertex.z += 0.75f;
    //inputVertex.y -= 0.75f;
    OutputToRasterizer output = { float4(0,0,0,1), inputVertex, nrm, uvw, 0, 0};
    
    //position Vector
    float4 outputVertex = float4(inputVertex, 1);
    output.posW = inputVertex;
    
    outputVertex = mul(outputVertex, mat_World[insID + int(tmIDS.x)]);
    output.posW = outputVertex;
    
    outputVertex = mul(outputVertex, mat_View);
    outputVertex = mul(outputVertex, mat_Projection);
    output.posH = outputVertex;
    
    output.normW = mul(nrm, mat_World[insID + int(tmIDS.x)]);
    output.instID = insID;

    return output;
}