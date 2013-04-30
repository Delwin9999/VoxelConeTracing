/*
 Copyright (c) 2012 The VCT Project

  This file is part of VoxelConeTracing and is an implementation of
  "Interactive Indirect Illumination Using Voxel Cone Tracing" by Crassin et al

  VoxelConeTracing is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  VoxelConeTracing is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with VoxelConeTracing.  If not, see <http://www.gnu.org/licenses/>.
*/

/*!
* \author Dominik Lazarek (dominik.lazarek@gmail.com)
* \author Andreas Weinmann (andy.weinmann@gmail.com)
*/

#include "VoxelConeTracing/VCTscene.h"
#include "VoxelConeTracing/Cube.h"

#include "KoRE/RenderManager.h"
#include "KoRE/ResourceManager.h"
#include "KoRE/GLerror.h"
#include "KoRE/Components/TexturesComponent.h"

VCTscene::VCTscene() :
  _camera(NULL),
  _tex3DclearPBO(KORE_GLUINT_HANDLE_INVALID),
  _voxelGridResolution(0),
  _voxelGridSideLengths(50, 50, 50) {
}

VCTscene::~VCTscene() {
  glDeleteBuffers(1, &_tex3DclearPBO);
}

void VCTscene::init(const SVCTparameters& params,
                    const std::vector<kore::SceneNode*>& meshNodes,
                    kore::Camera* camera) {
  _voxelGridResolution = params.voxel_grid_resolution;
  _voxelGridSideLengths = params.voxel_grid_sidelengths;

  _meshNodes = meshNodes;
  _camera = camera;

  _meshComponents.clear();
  _meshComponents.resize(_meshNodes.size());
  for (uint i = 0; i < _meshNodes.size(); ++i) {
    _meshComponents[i]
      = static_cast<kore::MeshComponent*>
      (_meshNodes[i]->getComponent(kore::COMPONENT_MESH));
  }
  
  initTex3D(&_voxelTex, BLACK);
  initVoxelFragList();


  _voxelGridNode = new kore::SceneNode;
  _voxelGridNode->scale(_voxelGridSideLengths / 2.0f, kore::SPACE_LOCAL);
  kore::SceneManager::getInstance()->
      getRootNode()->addChild(_voxelGridNode);

  kore::TexturesComponent* texComp = new kore::TexturesComponent;
  texComp->addTexture(&_voxelTex);
  _voxelGridNode->addComponent(texComp);

  Cube* voxelGridCube = new Cube(2.0f);
  kore::MeshComponent* meshComp = new kore::MeshComponent;
  meshComp->setMesh(voxelGridCube);
  _voxelGridNode->addComponent(meshComp);
}

void VCTscene::initVoxelFragList() {
  // Positions
  kore::STextureBufferProperties props;
  props.internalFormat = GL_R32UI;
  props.size = sizeof(unsigned int)
               * _voxelGridResolution
               * _voxelGridResolution
               * _voxelGridResolution * 2;
  props.usageHint = GL_DYNAMIC_COPY;

  _voxelFragLists[VOXELATT_POSITION]
                              .create(props, "VoxelFragmentList_Position");
  _voxelFragLists[VOXELATT_COLOR].create(props, "VoxelFragmentList_Color");

  
  _vflTexInfos[VOXELATT_POSITION].internalFormat = props.internalFormat;
  _vflTexInfos[VOXELATT_POSITION].texTarget = GL_TEXTURE_BUFFER;
  _vflTexInfos[VOXELATT_POSITION].texLocation
                        = _voxelFragLists[VOXELATT_POSITION].getTexHandle();

  _vflTexInfos[VOXELATT_COLOR].internalFormat = props.internalFormat;
  _vflTexInfos[VOXELATT_COLOR].texTarget = GL_TEXTURE_BUFFER;
  _vflTexInfos[VOXELATT_COLOR].texLocation
                       = _voxelFragLists[VOXELATT_COLOR].getTexHandle();
  

  _shdVoxelFragLists[VOXELATT_POSITION].component = NULL;
  _shdVoxelFragLists[VOXELATT_POSITION].data = &_vflTexInfos[VOXELATT_POSITION];
  _shdVoxelFragLists[VOXELATT_POSITION].name = "VoxelFragmentList_Position";
  _shdVoxelFragLists[VOXELATT_POSITION].type = GL_TEXTURE_BUFFER;

  _shdVoxelFragLists[VOXELATT_COLOR].component = NULL;
  _shdVoxelFragLists[VOXELATT_COLOR].data = &_vflTexInfos[VOXELATT_COLOR];
  _shdVoxelFragLists[VOXELATT_COLOR].name = "VoxelFragmentList_Color";
  _shdVoxelFragLists[VOXELATT_COLOR].type = GL_TEXTURE_BUFFER;
}

void VCTscene::clearTex3D(kore::Texture* tex) {
  kore::RenderManager::getInstance()->bindBuffer(GL_PIXEL_UNPACK_BUFFER,_tex3DclearPBO);
  for (uint z = 0; z < _voxelGridResolution; ++z) {
    kore::GLerror::gl_ErrorCheckStart();
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, z,
                    _voxelGridResolution,
                    _voxelGridResolution, 1,
                    GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    kore::GLerror::gl_ErrorCheckFinish("Upload 3D texture values");
  }
  kore::RenderManager::getInstance()->bindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
}

void VCTscene::initTex3D(kore::Texture* tex, const ETex3DContent texContent) {
  using namespace kore;

  unsigned int colorValues[128 * 128];
  
  /*unsigned int* colorValues
    = new unsigned int[_voxelGridResolution * _voxelGridResolution]; */
  memset(colorValues, 0, _voxelGridResolution * _voxelGridResolution * sizeof(unsigned int));

  GLerror::gl_ErrorCheckStart();
  glGenBuffers(1, &_tex3DclearPBO);
  RenderManager::getInstance()
      ->bindBuffer(GL_PIXEL_UNPACK_BUFFER, _tex3DclearPBO);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, 
               _voxelGridResolution * _voxelGridResolution * sizeof(unsigned int), 
               colorValues, GL_STATIC_DRAW);
  RenderManager::getInstance()
    ->bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  GLerror::gl_ErrorCheckFinish("Upload Pixel buffer values");
  
  _shdVoxelGridResolution.data = &_voxelGridResolution;
  _shdVoxelGridResolution.name = "VoxelGridResolution";
  _shdVoxelGridResolution.size = 1;
  _shdVoxelGridResolution.type = GL_UNSIGNED_INT;

  STextureProperties texProps;
  texProps.targetType = GL_TEXTURE_3D;
  texProps.width = _voxelGridResolution;
  texProps.height = _voxelGridResolution;
  texProps.depth = _voxelGridResolution;
  texProps.internalFormat = GL_R32UI;
  texProps.format = GL_RED_INTEGER;
  texProps.pixelType = GL_UNSIGNED_INT;

  tex->create(texProps, "voxelTexture");
  
  // Create data
  RenderManager::getInstance()->activeTexture(0);
  RenderManager::getInstance()->bindTexture(GL_TEXTURE_3D, tex->getHandle());

  //clearTex3D(tex);

  // TODO: For some reason, the pixelbuffer-approach doesn't work... so we'll
  // stick to manual clearing for now...
  kore::GLerror::gl_ErrorCheckStart();
  for (uint z = 0; z < _voxelGridResolution; ++z) {
    kore::GLerror::gl_ErrorCheckStart();
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, z,
      _voxelGridResolution,
      _voxelGridResolution, 1,
      GL_RED_INTEGER, GL_UNSIGNED_INT, colorValues);
  }
  kore::GLerror::gl_ErrorCheckFinish("Upload 3D texture values");
  
  
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_R, GL_RED);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  RenderManager::getInstance()->bindTexture(GL_TEXTURE_3D, 0);

  //delete[] colorValues;
}

