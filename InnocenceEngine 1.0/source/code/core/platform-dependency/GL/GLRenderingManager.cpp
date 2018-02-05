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
					// any normal?
					auto l_normalTextureID = l_textureMap.find(textureType::NORMAL);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update(0);
					}
					// any albedo?
					auto l_diffuseTextureID = l_textureMap.find(textureType::ALBEDO);
					if (l_diffuseTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
						l_textureData.update(1);
					}
					// any metallic?
					auto l_specularTextureID = l_textureMap.find(textureType::METALLIC);
					if (l_specularTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_specularTextureID->second)->second;
						l_textureData.update(2);
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

void GeometryPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

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
					// any normal?
					auto l_normalTextureID = l_textureMap.find(textureType::NORMAL);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update(0);
					}
					// any diffuse?
					auto l_diffuseTextureID = l_textureMap.find(textureType::ALBEDO);
					if (l_diffuseTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
						l_textureData.update(1);
					}
					// any specular?
					auto l_specularTextureID = l_textureMap.find(textureType::METALLIC);
					if (l_specularTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_specularTextureID->second)->second;
						l_textureData.update(2);
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

void LightPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents)
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

void GeometryPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

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
					auto& l_normalTextureID = l_textureMap.find(textureType::NORMAL);
					if (l_normalTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_normalTextureID->second)->second;
						l_textureData.update(0);
					}
					// any albedo?
					auto& l_albedoTextureID = l_textureMap.find(textureType::ALBEDO);
					if (l_albedoTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_albedoTextureID->second)->second;
						l_textureData.update(1);
					}
					// any metallic?
					auto& l_metallicTextureID = l_textureMap.find(textureType::METALLIC);
					if (l_metallicTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_metallicTextureID->second)->second;
						l_textureData.update(2);
					}
					// any roughness?
					auto& l_roughnessTextureID = l_textureMap.find(textureType::ROUGHNESS);
					if (l_roughnessTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_roughnessTextureID->second)->second;
						l_textureData.update(3);
					}
					// any ao?
					auto& l_aoTextureID = l_textureMap.find(textureType::AMBIENT_OCCLUSION);
					if (l_aoTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_aoTextureID->second)->second;
						l_textureData.update(4);
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
	updateUniform("uni_irradianceMap", 4);
}

void LightPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents)
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

void EnvironmentCapturePassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/environmentCapturePassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/environmentCapturePassPBSFragment.sf");
	bindShader();
	updateUniform("uni_equirectangularMap", 0);
}

void EnvironmentCapturePassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0f/ 180.0f) * PI, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))
	};

	bindShader();
	updateUniform("uni_p", captureProjection);

		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{

				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					twoDTextureMap.find(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second)->second.update();
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform("uni_r", captureViews[i]);
						threeDTexture.updateFramebuffer(i);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						meshMap.find(l_graphicData.first)->second.update();
					}
				}
			}
		}	
}


void EnvironmentConvolutionPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/environmentConvolutionPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/environmentConvolutionPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_capturedCubeMap", 0);
}

void EnvironmentConvolutionPassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDConvolutedTexture)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0f / 180.0f) * PI, 1.0f, 0.1f, 10.0f);

	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)),
		mat4().lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))
	};

	bindShader();
	updateUniform("uni_p", captureProjection);

	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
		{

			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				threeDCapturedTexture.update();
				for (unsigned int i = 0; i < 6; ++i)
				{
					updateUniform("uni_r", captureViews[i]);
					threeDConvolutedTexture.updateFramebuffer(i);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					meshMap.find(l_graphicData.first)->second.update();
				}
			}
		}
	}
}

void SkyForwardPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/skyForwardPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/skyForwardPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_skybox", 0);
}

void SkyForwardPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDTexture)
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
				threeDTexture.update();
				meshMap.find(l_graphicData.first)->second.update();
			}
		}
	}

	glDisable(GL_CULL_FACE);

	glDepthFunc(GL_LESS);
}

void DebuggerShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/debuggerVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");

	addShader(GLShader::GEOMETRY, "GL3.3/debuggerGeometry.sf");

	addShader(GLShader::FRAGMENT, "GL3.3/debuggerFragment.sf");

	bindShader();
}

void DebuggerShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

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


void SkyDeferPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/skyDeferPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/skyDeferPassPBSFragment.sf");
	bindShader();
	updateUniform("uni_lightPassRT0", 0);
	updateUniform("uni_skyForwardPassRT0", 1);
	updateUniform("uni_debuggerPassRT0", 2);
}

void SkyDeferPassPBSShader::shaderDraw()
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
	updateUniform("uni_skyDeferPassRT0", 0);
}

void FinalPassShader::shaderDraw()
{
	bindShader();
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

textureID GLRenderingManager::add2DHDRTexture()
{
	GL2DHDRTexture new2DHDRTexture;
	m_2DHDRTextureMap.emplace(std::pair<textureID, GL2DHDRTexture>(new2DHDRTexture.getEntityID(), new2DHDRTexture));
	return new2DHDRTexture.getEntityID();
}

textureID GLRenderingManager::add3DTexture()
{
	GL3DTexture new3DTexture;
	m_3DTextureMap.emplace(std::pair<textureID, GL3DTexture>(new3DTexture.getEntityID(), new3DTexture));
	return new3DTexture.getEntityID();
}

textureID GLRenderingManager::add3DHDRTexture()
{
	GL3DHDRTexture new3DHDRTexture;
	m_3DHDRTextureMap.emplace(std::pair<textureID, GL3DHDRTexture>(new3DHDRTexture.getEntityID(), new3DHDRTexture));
	return new3DHDRTexture.getEntityID();
}

IMesh* GLRenderingManager::getMesh(meshID meshID)
{
	return &m_meshMap.find(meshID)->second;
}

I2DTexture* GLRenderingManager::get2DTexture(textureID textureID)
{
	return &m_2DTextureMap.find(textureID)->second;
}

I2DTexture * GLRenderingManager::get2DHDRTexture(textureID textureID)
{
	return &m_2DHDRTextureMap.find(textureID)->second;;
}

I3DTexture * GLRenderingManager::get3DTexture(textureID textureID)
{
	return &m_3DTextureMap.find(textureID)->second;
}

I3DTexture * GLRenderingManager::get3DHDRTexture(textureID textureID)
{
	return &m_3DHDRTextureMap.find(textureID)->second;;
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
	renderBackgroundPass(cameraComponents, lightComponents, visibleComponents);
	renderGeometryPass(cameraComponents, lightComponents, visibleComponents);
	renderLightPass(cameraComponents, lightComponents, visibleComponents);
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

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_geometryPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_geometryPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_geometryPassRBO);

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
	m_geometryPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap, m_2DTextureMap);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLRenderingManager::initializeLightPass()
{
	// initialize shader
	m_lightPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_lightPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightPassFBO);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_lightPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_lightPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_lightPassRBO);

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

	m_3DHDRTextureMap.find(m_environmentConvolutionPassTextureID)->second.update(4);

	m_lightPassShader->shaderDraw(cameraComponents, lightComponents);
	// draw light pass rectangle
	glBindVertexArray(m_lightPassVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::initializeBackgroundPass()
{
	// environment capture pass
	m_environmentCapturePassShader->init();

	glGenFramebuffers(1, &m_environmentPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_environmentPassFBO);

	glGenRenderbuffers(1, &m_environmentPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_environmentPassRBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_environmentPassRBO);

	// @TODO: add a capturer class
	m_environmentCapturePassTextureID = this->add3DHDRTexture();
	auto environmentCapturePassTextureData = this->get3DHDRTexture(m_environmentCapturePassTextureID);
	environmentCapturePassTextureData->setup(textureType::CUBEMAP_HDR, 3, 2048, 2048, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
	environmentCapturePassTextureData->initialize();

	// environment convolution pass
	m_environmentConvolutionPassShader->init();

	m_environmentConvolutionPassTextureID = this->add3DHDRTexture();
	auto environmentConvolutionPassTextureData = this->get3DHDRTexture(m_environmentConvolutionPassTextureID);
	environmentConvolutionPassTextureData->setup(textureType::CUBEMAP_HDR, 3, 128, 128, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
	environmentConvolutionPassTextureData->initialize();

	// background forward pass
	m_skyForwardPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_skyForwardPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_skyForwardPassFBO);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_skyForwardPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_skyForwardPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_skyForwardPassRBO);

	// final color buffer
	glGenTextures(1, &m_skyForwardPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_skyForwardPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_skyForwardPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments_forward[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments_forward);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Background Forward Pass Framebuffer not complete!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// initialize Debugger Pass shader
	m_debuggerPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_debuggerPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_debuggerPassFBO);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_debuggerPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_debuggerPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_debuggerPassRBO);

	// Debugger Pass color buffer
	glGenTextures(1, &m_debuggerPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_debuggerPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_debuggerPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments_debugger[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments_debugger);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Debugger Pass Framebuffer not complete!");
	}

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// background defer pass
	m_skyDeferPassShader->init();

	//generate and bind frame buffer
	glGenFramebuffers(1, &m_skyDeferPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_skyDeferPassFBO);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_skyDeferPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_skyDeferPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (int)m_screenResolution.x, (int)m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_skyDeferPassRBO);

	// final color buffer
	glGenTextures(1, &m_skyDeferPassRT0Texture);
	glBindTexture(GL_TEXTURE_2D, m_skyDeferPassRT0Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_screenResolution.x, (int)m_screenResolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_skyDeferPassRT0Texture, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments_defer[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments_defer);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Background Defer Pass Framebuffer not complete!");
	}

	// initialize background defer pass rectangle
	m_skyDeferPassVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_skyDeferPassVAO);
	glGenBuffers(1, &m_skyDeferPassVBO);
	glBindVertexArray(m_skyDeferPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_skyDeferPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_skyDeferPassVertices.size() * sizeof(float), &m_skyDeferPassVertices[0], GL_STATIC_DRAW);
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
	if (m_shouldUpdateEnvironmentMap)
	{
		// draw environment map capture pass
		glBindFramebuffer(GL_FRAMEBUFFER, m_environmentPassFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, m_environmentPassRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 2048, 2048);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		glViewport(0, 0, 2048, 2048);

		m_environmentCapturePassShader->shaderDraw(visibleComponents, m_meshMap, m_2DHDRTextureMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second);

		// draw environment map convolution pass
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
		glViewport(0, 0, 128, 128);

		m_environmentConvolutionPassShader->shaderDraw(visibleComponents, m_meshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second, m_3DHDRTextureMap.find(m_environmentConvolutionPassTextureID)->second);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_screenResolution.x, m_screenResolution.x);
		glViewport(0, 0, m_screenResolution.x, m_screenResolution.y);

		m_shouldUpdateEnvironmentMap = false;
	}

	// draw background forward pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_skyForwardPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_skyForwardPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_skyForwardPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second);

	// draw debugger pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_debuggerPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_debuggerPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_debuggerPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap);

	// draw background defer pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_skyDeferPassFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_skyDeferPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_lightPassRT0Texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_skyForwardPassRT0Texture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_debuggerPassRT0Texture);

	m_skyDeferPassShader->shaderDraw();

	// draw background defer pass rectangle
	glBindVertexArray(m_skyDeferPassVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::initializeFinalPass()
{
	// initialize Final Pass shader
	m_finalPassShader->init();

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
	// draw final pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_skyDeferPassRT0Texture);

	m_finalPassShader->shaderDraw();

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

	m_environmentCapturePassShader = &EnvironmentCapturePassPBSShader::getInstance();
	m_environmentConvolutionPassShader = &EnvironmentConvolutionPassPBSShader::getInstance();

	m_skyForwardPassShader = &SkyForwardPassPBSShader::getInstance();
	m_skyDeferPassShader = &SkyDeferPassPBSShader::getInstance();

	m_debuggerPassShader = &DebuggerShader::getInstance();

	m_finalPassShader = &FinalPassShader::getInstance();
}

void GLRenderingManager::initialize()
{
	//BillboardPassShader::getInstance().init();
	glEnable(GL_TEXTURE_2D);

	initializeGeometryPass();

	initializeLightPass();

	initializeBackgroundPass();

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

