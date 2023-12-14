
uint activeViewport;

struct OutputToRasterizer
{
    
    float4 posH : SV_POSITION;
    float3 posW : WORLD; //is this just the POSITION?
    float3 normW : NORMAL;
    float3 uvwW : UVW;
    uint instID : SV_InstanceID;
    uint viewport : SV_ViewportArrayIndex;
};


struct PS_INPUT
{
    OutputToRasterizer pInput;
};

cbuffer GSData
{
	float4 index;
};

[maxvertexcount(3)]
void main(triangle PS_INPUT In[3],
	inout TriangleStream<OutputToRasterizer> stream)
{
    OutputToRasterizer output;
    output.viewport = uint(index.x);
    for (int v = 0; v < 3; v++)
    {
        output.posH = In[v].pInput.posH;
        output.posW = In[v].pInput.posH;
        output.normW = In[v].pInput.normW;
        output.uvwW = In[v].pInput.uvwW;
        output.instID = In[v].pInput.instID;
        stream.Append(output);
    }        
}