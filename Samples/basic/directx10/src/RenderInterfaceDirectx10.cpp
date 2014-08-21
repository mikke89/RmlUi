#include "RenderInterfaceDirectX10.h"
#include <Rocket/Core.h>

#include "D3D10Effect.h"

//RocketD3D10 Texture, this contains the actual texture and the resource view
//for sending it to the effect
struct RocketD3D10Texture
{
	ID3D10ShaderResourceView * textureView;
	ID3D10Texture2D * texture2D;
};

// This structure is created for each set of geometry that Rocket compiles. It stores the vertex and index buffers and
// the texture associated with the geometry, if one was specified.
struct RocketD310DCompiledGeometry
{
	//Vertex Buffer
	ID3D10Buffer * vertices;
	DWORD num_vertices;

	//Index buffer
	ID3D10Buffer * indices;
	DWORD num_primitives;
	//Texture
	RocketD3D10Texture * texture;
};

// The internal format of the vertex we use for rendering Rocket geometry. We could optimise space by having a second
// untextured vertex for use when rendering coloured borders and backgrounds.
struct RocketD3D10Vertex
{
	FLOAT x, y, z;
	D3DXCOLOR colour;
	FLOAT u, v;
};

//The layout of the vertices
const D3D10_INPUT_ELEMENT_DESC layout[] =
{
    { "POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,28,D3D10_INPUT_PER_VERTEX_DATA,0},
};

//The constructor of the render
RenderInterfaceDirectX10::RenderInterfaceDirectX10(void)
{
	m_rocket_context = NULL;
	m_pD3D10Device = NULL;

	m_pEffect = NULL;
	m_pTechnique = NULL;
	m_pVertexLayout = NULL;

	m_pSwapChain = NULL;
	m_pRenderTargetView = NULL;

	m_pProjectionMatrixVariable = NULL;
	m_pWorldMatrixVariable = NULL;
	m_pDiffuseTextureVariable = NULL;

	m_pScissorTestDisable = NULL;
	m_pScissorTestEnable = NULL;
}

//Loads the effect from memory and retrieves initial variables from the effect
void RenderInterfaceDirectX10::setupEffect()
{
	//The pass we are going to use
	ID3D10EffectPass *pass=NULL;
	DWORD dwShaderFlags = 0;
	#if defined( DEBUG ) || defined( _DEBUG )
		// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program. - BMD
		dwShaderFlags |= D3D10_SHADER_DEBUG;

	#endif
	//Create our effect from Memory
	if (FAILED(D3DX10CreateEffectFromMemory((void*)pEffectData,strlen(pEffectData),"DefaultEffect",NULL,NULL,"fx_4_0",dwShaderFlags,0,m_pD3D10Device,NULL,NULL,&m_pEffect,NULL,NULL)))
	{
		//Log error
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Can't create default effect for rendering, graphics card may not support Shader Model 4");
		
	}
	
	//Number of elements in the layout - BMD
	UINT numElements = sizeof(layout)/sizeof(D3D10_INPUT_ELEMENT_DESC);
	//Get the pass description so we can get some info about the input signature of the vertices
	D3D10_PASS_DESC passDesc;
	m_pTechnique=m_pEffect->GetTechniqueByName("Render");
	pass=m_pTechnique->GetPassByName("P0");
	pass->GetDesc(&passDesc);
	//create input layout, to allow us to map our vertex structure to the one held in the effect
	if (FAILED(m_pD3D10Device->CreateInputLayout(layout, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pVertexLayout)))
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Unable to create input layout");
	}
	//grab effect variables
	m_pWorldMatrixVariable=m_pEffect->GetVariableByName("matWorld")->AsMatrix();
	m_pProjectionMatrixVariable=m_pEffect->GetVariableByName("matProjection")->AsMatrix();

	//grab texture variable
	m_pDiffuseTextureVariable=m_pEffect->GetVariableByName("diffuseMap")->AsShaderResource();
}

RenderInterfaceDirectX10::~RenderInterfaceDirectX10()
{
	if (m_pVertexLayout)
	{
		m_pVertexLayout->Release();
		m_pVertexLayout = NULL;
	}
	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = NULL;
	}
	if (m_pScissorTestDisable){
		m_pScissorTestDisable->Release();
		m_pScissorTestDisable = NULL;
	}
	if (m_pScissorTestEnable){
		m_pScissorTestEnable->Release();
		m_pScissorTestEnable = NULL;
	}
}

// Called by Rocket when it wants to render geometry that it does not wish to optimise.
void RenderInterfaceDirectX10::RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
{
	// @TODO We've chosen to not support non-compiled geometry in the DirectX renderer. If you wanted to render non-compiled
	// geometry, for example for very small sections of geometry, you could use DrawIndexedPrimitiveUP or write to a
	// dynamic vertex buffer which is flushed when either the texture changes or compiled geometry is drawn.

        /// @TODO, HACK, just use the compiled geometry framework for now, this is inefficient but better than absolutely nothing
        /// for the time being
	Rocket::Core::CompiledGeometryHandle geom = this->CompileGeometry(vertices, num_vertices, indices, num_indices, texture);
	this->RenderCompiledGeometry(geom, translation);
	this->ReleaseCompiledGeometry(geom);
}

// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
Rocket::Core::CompiledGeometryHandle RenderInterfaceDirectX10::CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture)
{
	//Create instance of geometry
	RocketD310DCompiledGeometry * geometry =new RocketD310DCompiledGeometry();

	//Vertex Buffer description
	D3D10_BUFFER_DESC bd;
	bd.Usage = D3D10_USAGE_DEFAULT;
	//Set the size of the buffer
	bd.ByteWidth = sizeof(RocketD3D10Vertex) * num_vertices;
	//This is a vertex buffer
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	//copy vertices into buffer
	RocketD3D10Vertex * pD3D10Vertices=new RocketD3D10Vertex[num_vertices];
	for (int i=0;i<num_vertices;++i)
	{
		pD3D10Vertices[i].x = vertices[i].position.x;
		pD3D10Vertices[i].y = vertices[i].position.y;
		pD3D10Vertices[i].z = 0;

		pD3D10Vertices[i].colour=D3DXCOLOR((float)(vertices[i].colour.red/255), (float)(vertices[i].colour.green/255), (float)(vertices[i].colour.blue/255), 
			(float)(vertices[i].colour.alpha/255));
		
		pD3D10Vertices[i].u = vertices[i].tex_coord[0];
		pD3D10Vertices[i].v = vertices[i].tex_coord[1];
	}
	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = pD3D10Vertices;

	//Create VB
	if (FAILED(m_pD3D10Device->CreateBuffer(
		&bd,
		&InitData,
		&geometry->vertices))){
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Undable to create vertex buffer for geometry");
		return false;
	}

	delete pD3D10Vertices;

	//Index buffer desc
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( UINT ) * num_indices;
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	//Index values
	InitData.pSysMem = indices;
	//Fill and create buffer
	if (FAILED(m_pD3D10Device->CreateBuffer(
		&bd,
		&InitData,
		&geometry->indices))){
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Undable to create index buffer for geometry");
		return false;
	}

	//save some info in the instance of the structure
	geometry->num_vertices = (DWORD) num_vertices;
	geometry->num_primitives = (DWORD) num_indices / 3;

	geometry->texture = texture == NULL ? NULL : (RocketD3D10Texture *) texture;

	return (Rocket::Core::CompiledGeometryHandle)geometry;
}

// Called by Rocket when it wants to render application-compiled geometry.
void RenderInterfaceDirectX10::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation)
{
	//Cast to D3D10 geometry
	RocketD310DCompiledGeometry* d3d10_geometry = (RocketD310DCompiledGeometry*) geometry;
	
	//if we have a texture then send it, notice we are sending the view(Shader resource) to the 
	//effect
	if (d3d10_geometry->texture)
		m_pDiffuseTextureVariable->SetResource(d3d10_geometry->texture->textureView);
	else
		m_pDiffuseTextureVariable->SetResource(NULL);
	
	//build and send the world matrix
	D3DXMatrixTranslation(&m_matWorld, translation.x, translation.y, 0);
	m_pWorldMatrixVariable->SetMatrix((float*)m_matWorld);
	//Set the layout of the vertices that are held in the VB
	m_pD3D10Device->IASetInputLayout(m_pVertexLayout);
	//Get the stride(size) of the a vertex, we need this to tell the pipeline the size of one vertex 
	UINT stride = sizeof(RocketD3D10Vertex);
	//The offset from start of the buffer to where our vertices are located 
	UINT offset = 0;
	//Set the VB we are using
	m_pD3D10Device->IASetVertexBuffers( 
							0, 
							1, 
							&d3d10_geometry->vertices, 
							&stride, 
							&offset );
	//Set the IB we are using
	m_pD3D10Device->IASetIndexBuffer(d3d10_geometry->indices,DXGI_FORMAT_R32_UINT,0);


	D3D10_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc( &techDesc );
	//Loop through the passes in the technique
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//Get a pass at current index and apply it
		m_pTechnique->GetPassByIndex( p )->Apply( 0 );
		//We are drawing trangle lists
		m_pD3D10Device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );		
							
		//Draw call
		m_pD3D10Device->DrawIndexed(d3d10_geometry->num_primitives*3,0,0);
	}
}

// Called by Rocket when it wants to release application-compiled geometry.
void RenderInterfaceDirectX10::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry)
{
	//Clean up after ourselves
	RocketD310DCompiledGeometry* d3d10_geometry=(RocketD310DCompiledGeometry*)geometry;

	if (d3d10_geometry->vertices){
		d3d10_geometry->vertices->Release();
		d3d10_geometry->vertices = NULL;
	}
	if (d3d10_geometry->indices){
		d3d10_geometry->indices->Release();
		d3d10_geometry->indices = NULL;
	}

	delete d3d10_geometry;
}

// Called by Rocket when it wants to enable or disable scissoring to clip content.
void RenderInterfaceDirectX10::EnableScissorRegion(bool enable)
{
	//Is the scissor test enabled?
	enable ? m_pD3D10Device->RSSetState(m_pScissorTestEnable) : m_pD3D10Device->RSSetState(m_pScissorTestDisable);
}

// Called by Rocket when it wants to change the scissor region.
void RenderInterfaceDirectX10::SetScissorRegion(int x, int y, int width, int height)
{
	//The scissor rect
	D3D10_RECT rect;
	rect.left=x;
	rect.right=x+width;
	rect.top=y;
	rect.bottom=y+height;

	m_pD3D10Device->RSSetScissorRects(1,&rect);
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1) 
struct TGAHeader 
{
	char  idLength;
	char  colourMapType;
	char  dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char  colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char  bitsPerPixel;
	char  imageDescriptor;
};
// Restore packing
#pragma pack()

// Called by Rocket when a texture is required by the library.
bool RenderInterfaceDirectX10::LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source)
{
	Rocket::Core::FileInterface* file_interface = Rocket::Core::GetFileInterface();
	Rocket::Core::FileHandle file_handle = file_interface->Open(source);
	if (file_handle == NULL)
		return false;

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);
	
	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));
	
	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures 
	
	if (header.dataType != 2)
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}
	
	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Only 24 and 32bit textures are supported");
		return false;
	}
	
	const char* image_src = buffer + sizeof(TGAHeader);
	unsigned char* image_dest = new unsigned char[image_size];
	
	// Targa is BGR, swap to RGB and flip Y axis
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * color_mode;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index+2];
			image_dest[write_index+1] = image_src[read_index+1];
			image_dest[write_index+2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index+3] = image_src[read_index+3];
			else
				image_dest[write_index+3] = 255;
			
			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;
	
	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);
	
	delete [] image_dest;
	delete [] buffer;
	
	return success;
}

// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
bool RenderInterfaceDirectX10::GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const byte* source, const Rocket::Core::Vector2i& source_dimensions)
{
	//Create the instance of our texture
	RocketD3D10Texture * pTexture=new RocketD3D10Texture();

	//Texture description
	D3D10_TEXTURE2D_DESC textureDesc;
	//Width and height of the texture
	textureDesc.Width=source_dimensions.x;
	textureDesc.Height=source_dimensions.y;
	//Mip levels
	textureDesc.MipLevels=1;
	textureDesc.ArraySize = 1;
	//the format of the texture
	textureDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
	//The access and usage of the texture
	textureDesc.CPUAccessFlags=D3D10_CPU_ACCESS_WRITE;
	textureDesc.Usage=D3D10_USAGE_DYNAMIC;
	//Our are we going to bind this texture to the pipeline
	textureDesc.BindFlags= D3D10_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags=0;
	textureDesc.SampleDesc.Count=1;
	textureDesc.SampleDesc.Quality=0;
	
	//create our texture
	if (FAILED(m_pD3D10Device->CreateTexture2D(&textureDesc,NULL,&pTexture->texture2D))){
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Unable to create texture");
		return false;
	}

	//now lets fill it
	D3D10_MAPPED_TEXTURE2D mappedTex;
	pTexture->texture2D->Map(D3D10CalcSubresource(0,0,1),D3D10_MAP_WRITE_DISCARD,0,&mappedTex);
	for (int y = 0; y < source_dimensions.y; ++y)
	{
		for (int x = 0; x < source_dimensions.x; ++x)
		{
			const byte* source_pixel = source + (source_dimensions.x * 4 * y) + (x * 4);
			byte* destination_pixel = ((byte*) mappedTex.pData) + mappedTex.RowPitch * y + x * 4;
			destination_pixel[0] = source_pixel[0];
			destination_pixel[1] = source_pixel[1];
			destination_pixel[2] = source_pixel[2];
			destination_pixel[3] = source_pixel[3];
		}
	}
	pTexture->texture2D->Unmap(D3D10CalcSubresource(0,0,1));

	//Create the shader resoure view for our texture, we need this
	//to send the texture to the effect
	D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format=textureDesc.Format;
	srvDesc.ViewDimension=D3D10_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels=textureDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip=0;
	if (FAILED(m_pD3D10Device->CreateShaderResourceView(pTexture->texture2D, &srvDesc, &pTexture->textureView))){
		Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR, "Unable to create texture view");
		return false;
	}

	texture_handle = (Rocket::Core::TextureHandle)pTexture;
	return true;
}

// Called by Rocket when a loaded texture is no longer required.
void RenderInterfaceDirectX10::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
	//clean up after ourselves
	RocketD3D10Texture * pTexture = (RocketD3D10Texture*)texture_handle;
	if (pTexture->texture2D){
		pTexture->texture2D->Release();
		pTexture->texture2D = NULL;
	}
	if (pTexture->textureView){
		pTexture->textureView->Release();
		pTexture->textureView = NULL;
	}
	delete pTexture;
}

// Returns the native horizontal texel offset for the renderer.
float RenderInterfaceDirectX10::GetHorizontalTexelOffset()
{
	return 0.0f;
}

// Returns the native vertical texel offset for the renderer.
float RenderInterfaceDirectX10::GetVerticalTexelOffset()
{
	return 0.0f;
}

