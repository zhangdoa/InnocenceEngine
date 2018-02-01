#include "../../../main/stdafx.h"
#include "GLRenderingManager.h"


GLShader::GLShader()
{
}

inline void GLShader::addShader(shaderType shaderType, const std::string & fileLocation) const
{
	attachShader(shaderType, AssetManager::getInstance().loadShader(fileLocation), m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Attribute lost: " + arrtributeName);
	}
}

inline void GLShader::bindShader() const
{
	glUseProgram(m_program);
}

inline void GLShader::initProgram()
{
	m_program = glCreateProgram();
}

inline void GLShader::addUniform(std::string uniform) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Uniform lost: " + uniform);
	}
}

inline void GLShader::updateUniform(const std::string & uniformName, bool uniformValue) const
{
	glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), (int)uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, int uniformValue) const
{
	glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, float uniformValue) const
{
	glUniform1f(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y) const
{
	glUniform2f(glGetUniformLocation(m_program, uniformName.c_str()), x, y);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y, float z, float w)
{
	glUniform4f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z, w);
}

inline void GLShader::updateUniform(const std::string & uniformName, const mat4 & mat) const
{
	glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &mat.m[0][0]);
}


GLShader::~GLShader()
{
}

inline void GLShader::attachShader(shaderType shaderType, const std::string& shaderFileContent, int m_program) const
{
	int l_glShaderType = 0;

	switch (shaderType)
	{
	case VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: LogManager::getInstance().printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);

	if (l_shader == 0) {
		LogManager::getInstance().printLog("Shader creation failed: memory location invaild when adding shader!");
	}

	char const * sourcePointer = shaderFileContent.c_str();
	glShaderSource(l_shader, 1, &sourcePointer, NULL);

	GLint Result = GL_FALSE;
	int l_infoLogLength;

	glGetShaderiv(m_program, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(m_program, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (l_infoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(m_program, l_infoLogLength, NULL, &ShaderErrorMessage[0]);
		LogManager::getInstance().printLog(&ShaderErrorMessage[0]);
	}

	glAttachShader(m_program, l_shader);

	compileShader();
	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shader, 1024, NULL, infoLog);
		std::cout << "Shader compile error: " << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
	}
	detachShader(l_shader);
}

inline void GLShader::compileShader() const
{
	glLinkProgram(m_program);

	glValidateProgram(m_program);
	LogManager::getInstance().printLog("Shader is compiled.");
}

inline void GLShader::detachShader(int shader) const
{
	//glDetachShader(m_program, shader);
	glDeleteShader(shader);
}

void BillboardPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/billboardPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/billboardPassFragment.sf");
	bindShader();
	updateUniform("uni_texture", 0);
}

void BillboardPassShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();
	mat4 m = mat4();

	// @TODO: multiply with inverse of camera rotation matrix
	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform("uni_m", l_visibleComponent->getParentActor()->caclTransformationMatrix());

			// draw each graphic data of visibleComponent
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				//active and bind textures
				// is there any texture?
				auto l_textureMap = l_graphicData.second;
				if (&l_textureMap != nullptr)
				{
					// any diffuse?
					auto l_diffuseTextureID = l_textureMap.find(textureType::DIFFUSE);
					if (l_diffuseTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
						l_textureData.update();
					}
					// any specular?
					auto l_specularTextureID = l_textureMap.find(textureType::SPECULAR);
					if (l_specularTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_specularTextureID->second)->second;
						l_textureData.update();
					}
					// any normal?
					auto l_normalTextureID = l_textureMap.find(textureType::NORMALS);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update();
					}
				}
				// draw meshes
				meshMap.find(l_graphicData.first)->second.update();
			}
		}
	}
}

void GeometryPassBlinnPhongShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/geometryPassBlinnPhongVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");

	addShader(GLShader::FRAGMENT, "GL3.3/geometryPassBlinnPhongFragment.sf");
	bindShader();

	updateUniform("uni_normalTexture", 0);
	updateUniform("uni_diffuseTexture", 1);
	updateUniform("uni_specularTexture", 2);

}

void GeometryPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();
	mat4 m = mat4();

	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform("uni_m", l_visibleComponent->getParentActor()->caclTransformationMatrix());

			// draw each graphic data of visibleComponent
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				//active and bind textures
				// is there any texture?
				auto l_textureMap = l_graphicData.second;
				if (&l_textureMap != nullptr)
				{
					// any diffuse?
					auto l_diffuseTextureID = l_textureMap.find(textureType::DIFFUSE);
					if (l_diffuseTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
						l_textureData.update();
					}
					// any specular?
					auto l_specularTextureID = l_textureMap.find(textureType::SPECULAR);
					if (l_specularTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_specularTextureID->second)->second;
						l_textureData.update();
					}
					// any normal?
					auto l_normalTextureID = l_textureMap.find(textureType::NORMALS);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update();
					}
				}
				// draw meshes
				meshMap.find(l_graphicData.first)->second.update();
			}
		}
	}
}

void LightPassBlinnPhongShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/lightPassBlinnPhongVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");

	addShader(GLShader::FRAGMENT, "GL3.3/lightPassBlinnPhongFragment.sf");
	bindShader();
	updateUniform("uni_RT0", 0);
	updateUniform("uni_RT1", 1);
	updateUniform("uni_RT2", 2);
	updateUniform("uni_RT3", 3);
}

void LightPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
	{
		//@TODO: generalization

		updateUniform("uni_viewPos", cameraComponents[0]->getParentActor()->getTransform()->getPos().x, cameraComponents[0]->getParentActor()->getTransform()->getPos().y, cameraComponents[0]->getParentActor()->getTransform()->getPos().z);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform("uni_dirLight.direction", lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform("uni_dirLight.color", lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			updateUniform("uni_pointLights[" + ss.str() + "].position", lightComponents[i]->getParentActor()->getTransform()->getPos().x, lightComponents[i]->getParentActor()->getTransform()->getPos().y, lightComponents[i]->getParentActor()->getTransform()->getPos().z);
			updateUniform("uni_pointLights[" + ss.str() + "].radius", lightComponents[i]->getRadius());
			updateUniform("uni_pointLights[" + ss.str() + "].color", lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
	}
}

void GeometryPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/geometryPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");

	addShader(GLShader::FRAGMENT, "GL3.3/geometryPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_normalTexture", 0);
	updateUniform("uni_albedoTexture", 1);
	updateUniform("uni_metallicTexture", 2);
	updateUniform("uni_roughnessTexture", 3);
	updateUniform("uni_aoTexture", 4);
}

void GeometryPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();
	mat4 m = mat4();

	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform("uni_m", l_visibleComponent->getParentActor()->caclTransformationMatrix());

			// draw each graphic data of visibleComponent
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				//active and bind textures
				// is there any texture?
				auto& l_textureMap = l_graphicData.second;
				if (&l_textureMap != nullptr)
				{
					// any normal?
					auto& l_normalTextureID = l_textureMap.find(textureType::NORMALS);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update();
					}
					// any albedo?
					auto& l_albedoTextureID = l_textureMap.find(textureType::DIFFUSE);
					if (l_albedoTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_albedoTextureID->second)->second;
						l_textureData.update();
					}
					// any metallic?
					auto& l_metallicTextureID = l_textureMap.find(textureType::SPECULAR);
					if (l_metallicTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_metallicTextureID->second)->second;
						l_textureData.update();
					}
					// any roughness?
					auto& l_roughnessTextureID = l_textureMap.find(textureType::AMBIENT);
					if (l_roughnessTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_roughnessTextureID->second)->second;
						l_textureData.update();
					}
					// any ao?
					auto& l_aoTextureID = l_textureMap.find(textureType::EMISSIVE);
					if (l_aoTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_aoTextureID->second)->second;
						l_textureData.update();
					}
				}
				// draw meshes
				meshMap.find(l_graphicData.first)->second.update();
			}
		}
	}
}

void LightPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/lightPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");

	addShader(GLShader::FRAGMENT, "GL3.3/lightPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_geometryPassRT0", 0);
	updateUniform("uni_geometryPassRT1", 1);
	updateUniform("uni_geometryPassRT2", 2);
	updateUniform("uni_geometryPassRT3", 3);
}

void LightPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
	{
		//@TODO: generalization

		updateUniform("uni_viewPos", cameraComponents[0]->getParentActor()->getTransform()->getPos().x, cameraComponents[0]->getParentActor()->getTransform()->getPos().y, cameraComponents[0]->getParentActor()->getTransform()->getPos().z);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform("uni_dirLight.direction", lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform("uni_dirLight.color", lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			updateUniform("uni_pointLights[" + ss.str() + "].position", lightComponents[i]->getParentActor()->getTransform()->getPos().x, lightComponents[i]->getParentActor()->getTransform()->getPos().y, lightComponents[i]->getParentActor()->getTransform()->getPos().z);
			updateUniform("uni_pointLights[" + ss.str() + "].radius", lightComponents[i]->getRadius());
			updateUniform("uni_pointLights[" + ss.str() + "].color", lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
	}
}

void BackgroundFPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/backgroundFPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/backgroundFPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_skybox", 0);
}

void BackgroundFPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL3DTexture>& textureMap)
{
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	bindShader();

	// TODO: fix "looking outside" problem// almost there
	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();

	updateUniform("uni_p", p);
	updateUniform("uni_r", r);

	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
		{
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				meshMap.find(l_graphicData.first)->second.update();
				textureMap.find(l_graphicData.second.find(textureType::CUBEMAP)->second)->second.update();
			}
		}
	}

	glDisable(GL_CULL_FACE);

	glDepthFunc(GL_LESS);
}

void BackgroundDPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/backgroundDPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/backgroundDPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_lightPassRT0", 0);
	updateUniform("uni_backgroundFPassRT0", 1);
}

void BackgroundDPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL3DTexture>& textureMap)
{
	bindShader();
}

void FinalPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/finalPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");

	addShader(GLShader::FRAGMENT, "GL3.3/finalPassFragment.sf");
	bindShader();
	updateUniform("uni_backgroundPassRT0", 0);
}

void FinalPassShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();
}

void DebuggerShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/debuggerVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");
	setAttributeLocation(3, "in_Tangent");
	setAttributeLocation(4, "in_Bitangent");

	addShader(GLShader::GEOMETRY, "GL3.3/debuggerGeometry.sf");

	addShader(GLShader::FRAGMENT, "GL3.3/debuggerFragment.sf");

	bindShader();
}

void DebuggerShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();
	mat4 m = mat4();

	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform("uni_m", l_visibleComponent->getParentActor()->caclTransformationMatrix());

			// draw each graphic data of visibleComponent
			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				// draw meshes
				meshMap.find(l_graphicData.first)->second.update();
			}
		}
	}
}

meshID GLRenderingManager::addMesh()
{
	GLMesh newMesh;
	m_meshMap.emplace(std::pair<meshID, GLMesh>(newMesh.getEntityID(), newMesh));
	return newMesh.getEntityID();
}

textureID GLRenderingManager::add2DTexture()
{
	GL2DTexture new2DTexture;
	m_2DTextureMap.emplace(std::pair<textureID, GL2DTexture>(new2DTexture.getEntityID(), new2DTexture));
	return new2DTexture.getEntityID();
}

textureID GLRenderingManager::add3DTexture()
{
	GL3DTexture new3DTexture;
	m_3DTextureMap.emplace(std::pair<textureID, GL3DTexture>(new3DTexture.getEntityID(), new3DTexture));
	return new3DTexture.getEntityID();
}

IMesh* GLRenderingManager::getMesh(meshID meshID)
{
	return &m_meshMap.find(meshID)->second;
}

I2DTexture* GLRenderingManager::get2DTexture(textureID textureID)
{
	return &m_2DTextureMap.find(textureID)->second;
}

I3DTexture * GLRenderingManager::get3DTexture(textureID textureID)
{
	return &m_3DTextureMap.find(textureID)->second;
}

void GLRenderingManager::forwardRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	// draw billboard
	BillboardPassShader::getInstance().shaderDraw(cameraComponents, lightComponents, visibleComponents, meshMap, textureMap);
	// draw skybox
	//SkyboxShader::getInstance().shaderDraw(cameraComponents, lightComponents, visibleComponents, meshMap, textureMap);
}

void GLRenderingManager::deferRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	renderGeometryPass(cameraComponents, lightComponents, visibleComponents);
	renderLightPass(cameraComponents, lightComponents, visibleComponents);
	renderBackgroundPass(cameraComponents, lightComponents, visibleComponents);
	renderFinalPass(cameraComponents, lightComponents, visibleComponents);
}

void GLRenderingManager::setScreenResolution(vec2 screenResolution)
{
	m_screenResolution = screenResolution;
}

void GLRenderingManager::initializeGeometryPass()
{
	// initialize shader
	m_geometryPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_geometryPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_geometryPassFBO);

	// position color buffer
	glGenTextures(1, &m_geometryPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_geometryPassRT0Texture, 0);

	// normal color buffer
	glGenTextures(1, &m_geometryPassRT1Texture);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT1Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_geometryPassRT1Texture, 0);

	// albedo color buffer
	glGenTextures(1, &m_geometryPassRT2Texture);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT2Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_geometryPassRT2Texture, 0);

	// metallic +  roughness + ao buffer
	glGenTextures(1, &m_geometryPassRT3Texture);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT3Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_geometryPassRT3Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_geometryPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_geometryPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_geometryPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Geometry Pass Framebuffer not complete!");
	}
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingManager::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_geometryPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_geometryPassRBO);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL - m_polygonMode);
	m_geometryPassShader->shaderDraw(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_2DTextureMap);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLRenderingManager::initializeLightPass()
{
	// initialize shader
	m_lightPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_lightPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightPassFBO);

	// final color buffer
	glGenTextures(1, &m_lightPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_lightPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_lightPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_lightPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_lightPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Light Pass Framebuffer not complete!");
	}

	// initialize light pass rectangle
	m_lightPassVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_lightPassVAO);
	glGenBuffers(1, &m_lightPassVBO);
	glBindVertexArray(m_lightPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_lightPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_lightPassVertices.size() * sizeof(float), &m_lightPassVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingManager::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_lightPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT0Texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT1Texture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT2Texture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassRT3Texture);
	
	m_lightPassShader->shaderDraw(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_2DTextureMap);
	// draw light pass rectangle
	glBindVertexArray(m_lightPassVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::initializeBackgroundPass()
{
	// background forward pass
	m_backgroundFPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_backgroundFPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_backgroundFPassFBO);

	// final color buffer
	glGenTextures(1, &m_backgroundFPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_backgroundFPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_backgroundFPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments_forward[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments_forward);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_backgroundFPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_backgroundFPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_backgroundFPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Background Forward Pass Framebuffer not complete!");
	}

	// initialize background forward pass rectangle
	m_backgroundFPassVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_backgroundFPassVAO);
	glGenBuffers(1, &m_backgroundFPassVBO);
	glBindVertexArray(m_backgroundFPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_backgroundFPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_backgroundFPassVertices.size() * sizeof(float), &m_backgroundFPassVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// background defer pass
	m_backgroundDPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_backgroundDPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_backgroundDPassFBO);

	// final color buffer
	glGenTextures(1, &m_backgroundDPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_backgroundDPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_backgroundDPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments_defer[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments_defer);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_backgroundDPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_backgroundDPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_backgroundDPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Background Defer Pass Framebuffer not complete!");
	}

	// initialize background defer pass rectangle
	m_backgroundDPassVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_backgroundDPassVAO);
	glGenBuffers(1, &m_backgroundDPassVBO);
	glBindVertexArray(m_backgroundDPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_backgroundDPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_backgroundDPassVertices.size() * sizeof(float), &m_backgroundDPassVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingManager::renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	// draw background forward pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_backgroundFPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_backgroundFPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_backgroundFPassShader->shaderDraw(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_3DTextureMap);

	// draw background defer pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_backgroundDPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_backgroundDPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_lightPassRT0Texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_backgroundFPassRT0Texture);

	m_backgroundDPassShader->shaderDraw(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_3DTextureMap);

	// draw background defer pass rectangle
	glBindVertexArray(m_backgroundDPassVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::initializeFinalPass()
{
	// initialize shader
	FinalPassShader::getInstance().init();

	// initialize screen rectangle
	m_screenVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_finalPassVAO);
	glGenBuffers(1, &m_finalPassVBO);
	glBindVertexArray(m_finalPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_finalPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_screenVertices.size() * sizeof(float), &m_screenVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLRenderingManager::renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_backgroundDPassRT0Texture);

	FinalPassShader::getInstance().shaderDraw(cameraComponents, lightComponents, visibleComponents, m_meshMap, m_2DTextureMap);

	//DebuggerShader::getInstance().shaderDraw(cameraComponents, lightComponents, visibleComponents, meshMap, textureMap);

	// draw screen rectangle
	glBindVertexArray(m_finalPassVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::changeDrawPolygonMode()
{
	if (m_polygonMode == 2)
	{
		m_polygonMode = 0;
	}
	else
	{
		m_polygonMode += 1;
	}
}

void GLRenderingManager::changeDrawTextureMode()
{
	if (m_textureMode == 3)
	{
		m_textureMode = 0;
	}
	else
	{
		m_textureMode += 1;
	}
}


void GLRenderingManager::setup()
{
	//@TODO: add a switch for different shader model
	//m_geometryPassShader = &GeometryPassBlinnPhongShader::getInstance();
	m_geometryPassShader = &GeometryPassPBSShader::getInstance();

	//m_lightPassShader = &LightPassBlinnPhongShader::getInstance();
	m_lightPassShader = &LightPassPBSShader::getInstance();

	m_backgroundFPassShader = &BackgroundFPassPBSShader::getInstance();
	m_backgroundDPassShader = &BackgroundDPassPBSShader::getInstance();
}

void GLRenderingManager::initialize()
{
	//BillboardPassShader::getInstance().init();
	glEnable(GL_TEXTURE_2D);

	initializeGeometryPass();

	initializeLightPass();

	initializeBackgroundPass();

	DebuggerShader::getInstance().init();
	initializeFinalPass();

	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
}

void GLRenderingManager::shutdown()
{
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("GLRenderingManager has been shutdown.");
}


