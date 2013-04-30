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

#include "VoxelConeTracing/VoxelizePass.h"

#include "KoRE/ResourceManager.h"
#include "KoRE/Operations/ViewportOp.h"
#include "KoRE/Operations/EnableDisableOp.h"
#include "KoRE/Operations/ColorMaskOp.h"
#include "KoRE/Operations/OperationFactory.h"
#include "KoRE/Operations/BindOperations/BindUniform.h"
#include "KoRE/Components/TexturesComponent.h"
#include "KoRE/Operations/RenderMesh.h"

VoxelizePass::VoxelizePass(VCTscene* vctScene)
{
  using namespace kore;

  // Init Voxelize procedure
  //////////////////////////////////////////////////////////////////////////
  ShaderProgram* voxelizeShader = new ShaderProgram;
  voxelizeShader->
    loadShader("./assets/shader/VoxelConeTracing/voxelizeVert.shader",
    GL_VERTEX_SHADER);

  voxelizeShader->
    loadShader("./assets/shader/VoxelConeTracing/voxelizeGeom.shader",
    GL_GEOMETRY_SHADER);

  voxelizeShader->
    loadShader("./assets/shader/VoxelConeTracing/voxelizeFrag.shader",
    GL_FRAGMENT_SHADER);
  voxelizeShader->init();
  voxelizeShader->setName("voxelizeShader");
  ResourceManager::getInstance()->addShaderProgram(voxelizeShader);
  
  this->setShaderProgram(voxelizeShader);
  const std::vector<kore::SceneNode*>& vRenderNdoes = vctScene->getRenderNodes();

  for (uint i = 0; i < vRenderNdoes.size(); ++i) {
    NodePass* nodePass = new NodePass(vRenderNdoes[i]);
    this->addNodePass(nodePass);

   nodePass->addOperation(new ViewportOp(glm::ivec4(0, 0,
                                  vctScene->getVoxelGridResolution(),
                                  vctScene->getVoxelGridResolution())));

  nodePass
    ->addOperation(new EnableDisableOp(GL_DEPTH_TEST,
                                       EnableDisableOp::DISABLE));
  
  nodePass
    ->addOperation(new ColorMaskOp(glm::bvec4(false, false, false, false)));

   MeshComponent* meshComp =
     static_cast<MeshComponent*>(vRenderNdoes[i]->getComponent(COMPONENT_MESH));
   
   nodePass
     ->addOperation(OperationFactory::create(OP_BINDATTRIBUTE, "v_position",
                                             meshComp, "v_position",
                                             voxelizeShader));
   nodePass
     ->addOperation(new BindUniform(vctScene->getShdVoxelGridResolution(),
                                  voxelizeShader->getUniform("voxelTexSize")));

  nodePass
    ->addOperation(OperationFactory::create(OP_BINDATTRIBUTE, "v_normal",
                                            meshComp, "v_normal",
                                            voxelizeShader));


   nodePass
     ->addOperation(OperationFactory::create(OP_BINDATTRIBUTE, "v_uv0",
                                             meshComp, "v_uvw", voxelizeShader));

   nodePass
     ->addOperation(OperationFactory::create(OP_BINDUNIFORM, "model Matrix",
                                             vRenderNdoes[i]->getTransform(), "modelWorld",
                                             voxelizeShader));

   nodePass
     ->addOperation(OperationFactory::create(OP_BINDUNIFORM, "normal Matrix",
                                             vRenderNdoes[i]->getTransform(), "modelWorldNormal",
                                             voxelizeShader));
   nodePass
     ->addOperation(OperationFactory::create(OP_BINDIMAGETEXTURE,
                                      vctScene->getVoxelTex()->getName(),
                                      static_cast<TexturesComponent*> (vctScene->getVoxelGridNode()->getComponent(COMPONENT_TEXTURES)), "voxelTex",
                                      voxelizeShader));

   const TexturesComponent* texComp =
     static_cast<TexturesComponent*>(vRenderNdoes[i]->getComponent(COMPONENT_TEXTURES));
   const Texture* tex = texComp->getTexture(0);
   
   nodePass
     ->addOperation(OperationFactory::create(OP_BINDTEXTURE, tex->getName(),
                                      texComp, "diffuseTex", voxelizeShader));

   nodePass->addOperation(OperationFactory::create(OP_BINDUNIFORM,
                          "model Matrix", vctScene->getVoxelGridNode()->getTransform(),
                          "voxelGridTransform", voxelizeShader));

   nodePass->addOperation(OperationFactory::create(OP_BINDUNIFORM,
                          "inverse model Matrix", vctScene->getVoxelGridNode()->getTransform(),
                          "voxelGridTransformI", voxelizeShader));

   nodePass
     ->addOperation(new RenderMesh(meshComp));
  }
}


VoxelizePass::~VoxelizePass(void) {
}
