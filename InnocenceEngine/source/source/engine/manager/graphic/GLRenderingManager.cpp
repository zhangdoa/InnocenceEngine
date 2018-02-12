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

inline GLint GLShader::getUniformLocation(const std::string & uniformName) const
{
	return glGetUniformLocation(m_program, uniformName.c_str());
}

inline void GLShader::updateUniform(const GLint uniformLocation, bool uniformValue) const
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

inline void GLShader::updateUniform(const GLint uniformLocation, int uniformValue) const
{
	glUniform1i(uniformLocation, uniformValue);
}

inline void GLShader::updateUniform(const GLint uniformLocation, float uniformValue) const
{
	glUniform1f(uniformLocation, uniformValue);
}

inline void GLShader::updateUniform(const GLint uniformLocation, float x, float y) const
{
	glUniform2f(uniformLocation, x, y);
}

inline void GLShader::updateUniform(const GLint uniformLocation, float x, float y, float z) const
{
	glUniform3f(uniformLocation, x, y, z);
}

inline void GLShader::updateUniform(const GLint uniformLocation, float x, float y, float z, float w)
{
	glUniform4f(uniformLocation, x, y, z, w);
}

inline void GLShader::updateUniform(const GLint uniformLocation, const mat4 & mat) const
{
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
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
	int l_infoLogLength = 0;

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
	m_uni_texture = getUniformLocation("uni_texture");
	updateUniform(m_uni_texture, 0);
	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");

}

void BillboardPassShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

	// @TODO: multiply with inverse of camera rotation matrix
	updateUniform(m_uni_p, p);
	updateUniform(m_uni_r, r);
	updateUniform(m_uni_t, t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform(m_uni_m, l_visibleComponent->getParentActor()->caclTransformationMatrix());

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

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	updateUniform(m_uni_normalTexture, 0);
	m_uni_diffuseTexture = getUniformLocation("uni_diffuseTexture");
	updateUniform(m_uni_diffuseTexture, 1);
	m_uni_specularTexture = getUniformLocation("uni_specularTexture");
	updateUniform(m_uni_specularTexture, 2);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");

}

void GeometryPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

	updateUniform(m_uni_p, p);
	updateUniform(m_uni_r, r);
	updateUniform(m_uni_t, t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform(m_uni_m, l_visibleComponent->getParentActor()->caclTransformationMatrix());

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

	m_uni_RT0 = getUniformLocation("uni_RT0");
	updateUniform(m_uni_RT0, 0);
	m_uni_RT1 = getUniformLocation("uni_RT1");
	updateUniform(m_uni_RT1, 1);
	m_uni_RT2 = getUniformLocation("uni_RT2");
	updateUniform(m_uni_RT2, 2);
	m_uni_RT3 = getUniformLocation("uni_RT3");
	updateUniform(m_uni_RT3, 3);

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents)
{
	bindShader();

	if (!isPointLightUniformAdded)
	{
		int l_pointLightIndexOffset = 0;
		for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
		{
			if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
			}
			if (lightComponents[i]->getLightType() == lightType::POINT)
			{
				std::stringstream ss;
				ss << i + l_pointLightIndexOffset;
				m_uni_pointLights_position.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].position"));
				m_uni_pointLights_radius.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].radius"));
				m_uni_pointLights_color.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].color"));
			}
		}
		isPointLightUniformAdded = true;
	}

	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
	{
		//@TODO: generalization

		updateUniform(m_uni_viewPos, cameraComponents[0]->getParentActor()->getTransform()->getPos().x, cameraComponents[0]->getParentActor()->getTransform()->getPos().y, cameraComponents[0]->getParentActor()->getTransform()->getPos().z);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentActor()->getTransform()->getPos().x, lightComponents[i]->getParentActor()->getTransform()->getPos().y, lightComponents[i]->getParentActor()->getTransform()->getPos().z);
			updateUniform(m_uni_pointLights_radius[i + l_pointLightIndexOffset], lightComponents[i]->getRadius());
			updateUniform(m_uni_pointLights_color[i + l_pointLightIndexOffset], lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
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

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	updateUniform(m_uni_normalTexture, 0);
	m_uni_albedoTexture = getUniformLocation("uni_albedoTexture");
	updateUniform(m_uni_albedoTexture, 1);
	m_uni_metallicTexture = getUniformLocation("uni_metallicTexture");
	updateUniform(m_uni_metallicTexture, 2);
	m_uni_roughnessTexture = getUniformLocation("uni_roughnessTexture");
	updateUniform(m_uni_roughnessTexture, 3);
	m_uni_aoTexture = getUniformLocation("uni_aoTexture");
	updateUniform(m_uni_aoTexture, 4);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");

	m_uni_useTexture = getUniformLocation("uni_useTexture");
	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_MRA = getUniformLocation("uni_MRA");
}

void GeometryPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

	updateUniform(m_uni_p, p);
	updateUniform(m_uni_r, r);
	updateUniform(m_uni_t, t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform(m_uni_m, l_visibleComponent->getParentActor()->caclTransformationMatrix());

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
				updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
				updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
				updateUniform(m_uni_MRA, l_visibleComponent->m_MRA.x, l_visibleComponent->m_MRA.y, l_visibleComponent->m_MRA.z);
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

	m_uni_geometryPassRT0 = getUniformLocation("uni_geometryPassRT0");
	updateUniform(m_uni_geometryPassRT0, 0);
	m_uni_geometryPassRT1 = getUniformLocation("uni_geometryPassRT1");
	updateUniform(m_uni_geometryPassRT1, 1);
	m_uni_geometryPassRT2 = getUniformLocation("uni_geometryPassRT2");
	updateUniform(m_uni_geometryPassRT2, 2);
	m_uni_geometryPassRT3 = getUniformLocation("uni_geometryPassRT3");
	updateUniform(m_uni_geometryPassRT3, 3);
	m_uni_irradianceMap = getUniformLocation("uni_irradianceMap");
	updateUniform(m_uni_irradianceMap, 4);
	m_uni_preFiltedMap = getUniformLocation("uni_preFiltedMap");
	updateUniform(m_uni_preFiltedMap, 5);
	m_uni_brdfLUT = getUniformLocation("uni_brdfLUT");
	updateUniform(m_uni_brdfLUT, 6);

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents)
{
	bindShader();

	if (!isPointLightUniformAdded)
	{
		int l_pointLightIndexOffset = 0;
		for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
		{
			if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
			}
			if (lightComponents[i]->getLightType() == lightType::POINT)
			{
				std::stringstream ss;
				ss << i + l_pointLightIndexOffset;
				m_uni_pointLights_position.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].position"));
				m_uni_pointLights_radius.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].radius"));
				m_uni_pointLights_color.emplace_back(getUniformLocation("uni_pointLights[" + ss.str() + "].color"));
			}
		}
		isPointLightUniformAdded = true;
	}

	int l_pointLightIndexOffset = 0;
	for (auto i = (unsigned int)0; i < lightComponents.size(); i++)
	{
		//@TODO: generalization

		updateUniform(m_uni_viewPos, cameraComponents[0]->getParentActor()->getTransform()->getPos().x, cameraComponents[0]->getParentActor()->getTransform()->getPos().y, cameraComponents[0]->getParentActor()->getTransform()->getPos().z);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentActor()->getTransform()->getPos().x, lightComponents[i]->getParentActor()->getTransform()->getPos().y, lightComponents[i]->getParentActor()->getTransform()->getPos().z);
			updateUniform(m_uni_pointLights_radius[i + l_pointLightIndexOffset], lightComponents[i]->getRadius());
			updateUniform(m_uni_pointLights_color[i + l_pointLightIndexOffset], lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
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

	m_uni_equirectangularMap = getUniformLocation("uni_equirectangularMap");
	updateUniform(m_uni_equirectangularMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
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
	updateUniform(m_uni_p, captureProjection);

		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{

				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					twoDTextureMap.find(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second)->second.update();
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(m_uni_r, captureViews[i]);
						threeDTexture.updateFramebuffer(i, 0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						meshMap.find(l_graphicData.first)->second.update();
						threeDTexture.update(0);
						glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
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

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
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
	updateUniform(m_uni_p, captureProjection);

	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
		{

			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				threeDCapturedTexture.update();
				for (unsigned int i = 0; i < 6; ++i)
				{
					updateUniform(m_uni_r, captureViews[i]);
					threeDConvolutedTexture.updateFramebuffer(i, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					meshMap.find(l_graphicData.first)->second.update();
				}
			}
		}
	}
}

void EnvironmentPreFilterPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/environmentPreFilterPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/environmentPreFilterPassPBSFragment.sf");
	bindShader();

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");

	m_uni_roughness = getUniformLocation("uni_roughness");
}

void EnvironmentPreFilterPassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDPreFiltedTexture)
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
	updateUniform(m_uni_p, captureProjection);

	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
		{

			for (auto& l_graphicData : l_visibleComponent->getModelMap())
			{
				threeDCapturedTexture.update();
				unsigned int maxMipLevels = 5;
				for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
				{
					// reisze framebuffer according to mip-level size.
					unsigned int mipWidth = 128 * std::pow(0.5, mip);
					unsigned int mipHeight = 128 * std::pow(0.5, mip);

					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
					glViewport(0, 0, mipWidth, mipHeight);

					float roughness = (float)mip / (float)(maxMipLevels - 1);
					updateUniform(m_uni_roughness, roughness);
					for (unsigned int i = 0; i < 6; ++i)
					{
						updateUniform(m_uni_r, captureViews[i]);
						threeDPreFiltedTexture.updateFramebuffer(i, mip);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						meshMap.find(l_graphicData.first)->second.update();
					}
				}		
			}
		}
	}
}

void EnvironmentBRDFLUTPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/environmentBRDFLUTPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/environmentBRDFLUTPassPBSFragment.sf");
	bindShader();
}

void EnvironmentBRDFLUTPassPBSShader::shaderDraw()
{
	bindShader();
}

void SkyForwardPassPBSShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/skyForwardPassPBSVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/skyForwardPassPBSFragment.sf");
	bindShader();

	m_uni_skybox = getUniformLocation("uni_skybox");
	updateUniform(m_uni_skybox, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
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

	updateUniform(m_uni_p, p);
	updateUniform(m_uni_r, r);

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

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void DebuggerShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap)
{
	bindShader();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getRotMatrix();
	mat4 t = cameraComponents[0]->getPosMatrix();

	updateUniform(m_uni_p, p);
	updateUniform(m_uni_r, r);
	updateUniform(m_uni_t, t);

	// draw each visibleComponent
	for (auto& l_visibleComponent : visibleComponents)
	{
		if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH)
		{
			updateUniform(m_uni_m, l_visibleComponent->getParentActor()->caclTransformationMatrix());

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

	m_uni_lightPassRT0 = getUniformLocation("uni_lightPassRT0");
	updateUniform(m_uni_lightPassRT0, 0);
	m_uni_skyForwardPassRT0 = getUniformLocation("uni_skyForwardPassRT0");
	updateUniform(m_uni_skyForwardPassRT0, 1);
	m_uni_debuggerPassRT0 = getUniformLocation("uni_debuggerPassRT0");
	updateUniform(m_uni_debuggerPassRT0, 2);
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

	m_uni_skyDeferPassRT0 = getUniformLocation("uni_skyDeferPassRT0");
	updateUniform(m_uni_skyDeferPassRT0, 0);
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

void GLRenderingManager::Render(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
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
	m_geometryPassFrameBuffer.setup(m_screenResolution, false, 4);
	m_geometryPassFrameBuffer.initialize();
}

void GLRenderingManager::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	m_geometryPassFrameBuffer.update();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL - m_polygonMode);
	m_geometryPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap, m_2DTextureMap);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
	environmentCapturePassTextureData->setup(textureType::CUBEMAP_HDR, 3, 2048, 2048, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, false);
	environmentCapturePassTextureData->initialize();

	// environment convolution pass
	m_environmentConvolutionPassShader->init();

	m_environmentConvolutionPassTextureID = this->add3DHDRTexture();
	auto environmentConvolutionPassTextureData = this->get3DHDRTexture(m_environmentConvolutionPassTextureID);
	environmentConvolutionPassTextureData->setup(textureType::CUBEMAP_HDR, 3, 128, 128, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, false);
	environmentConvolutionPassTextureData->initialize();

	// environment pre-filter pass
	m_environmentPreFilterPassShader->init();
	
	m_environmentPreFilterPassTextureID = this->add3DHDRTexture();
	auto environmentPreFilterPassTextureData = this->get3DHDRTexture(m_environmentPreFilterPassTextureID);
	environmentPreFilterPassTextureData->setup(textureType::CUBEMAP_HDR, 3, 128, 128, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, true);
	environmentPreFilterPassTextureData->initialize();

	// environment brdf LUT pass
	m_environmentBRDFLUTPassShader->init();

	glGenTextures(1, &m_environmentBRDFLUTTexture);
	glBindTexture(GL_TEXTURE_2D, m_environmentBRDFLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// initialize background defer pass rectangle
	m_environmentBRDFLUTPassVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_environmentBRDFLUTPassVAO);
	glGenBuffers(1, &m_environmentBRDFLUTPassVBO);
	glBindVertexArray(m_environmentBRDFLUTPassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_environmentBRDFLUTPassVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_environmentBRDFLUTPassVertices.size() * sizeof(float), &m_environmentBRDFLUTPassVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// background forward pass
	m_skyForwardPassShader->init();
	m_skyForwardPassFrameBuffer.setup(m_screenResolution, false, 1);
	m_skyForwardPassFrameBuffer.initialize();

	// initialize Debugger Pass shader
	m_debuggerPassShader->init();
	m_debuggerPassFrameBuffer.setup(m_screenResolution, true, 1);
	m_debuggerPassFrameBuffer.initialize();

	// background defer pass
	m_skyDeferPassShader->init();
	m_skyDeferPassFrameBuffer.setup(m_screenResolution, true, 1);
	m_skyDeferPassFrameBuffer.initialize();
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

		// draw environment map pre-filter pass
		m_environmentPreFilterPassShader->shaderDraw(visibleComponents, m_meshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second, m_3DHDRTextureMap.find(m_environmentPreFilterPassTextureID)->second);

		// draw environment map BRDF LUT pass
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glViewport(0, 0, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_environmentBRDFLUTTexture, 0);

		m_environmentBRDFLUTPassShader->shaderDraw();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw environment map BRDF LUT rectangle
		glBindVertexArray(m_environmentBRDFLUTPassVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_screenResolution.x, m_screenResolution.x);
		glViewport(0, 0, m_screenResolution.x, m_screenResolution.y);

		m_shouldUpdateEnvironmentMap = false;
	}

	// draw background forward pass
	m_skyForwardPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_skyForwardPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second);

	// draw debugger pass
	m_debuggerPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_debuggerPassShader->shaderDraw(cameraComponents, visibleComponents, m_meshMap);

	// draw background defer pass
	m_skyDeferPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_lightPassFrameBuffer.activeTexture(0, 0);
	m_skyForwardPassFrameBuffer.activeTexture(1, 0);
	m_debuggerPassFrameBuffer.activeTexture(2, 0);

	m_skyDeferPassShader->shaderDraw();

	// draw background defer pass rectangle
	m_skyDeferPassFrameBuffer.drawMesh();
}

void GLRenderingManager::initializeLightPass()
{
	// initialize shader
	m_lightPassShader->init();
	m_lightPassFrameBuffer.setup(m_screenResolution, true, 1);
	m_lightPassFrameBuffer.initialize();
}

void GLRenderingManager::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	m_lightPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_geometryPassFrameBuffer.activeTexture(0, 0);
	m_geometryPassFrameBuffer.activeTexture(1, 1);
	m_geometryPassFrameBuffer.activeTexture(2, 2);
	m_geometryPassFrameBuffer.activeTexture(3, 3);

	m_3DHDRTextureMap.find(m_environmentConvolutionPassTextureID)->second.update(4);
	m_3DHDRTextureMap.find(m_environmentPreFilterPassTextureID)->second.update(5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, m_environmentBRDFLUTTexture);

	m_lightPassShader->shaderDraw(cameraComponents, lightComponents);

	// draw light pass rectangle
	m_lightPassFrameBuffer.drawMesh();
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

	m_skyDeferPassFrameBuffer.activeTexture(0, 0);

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
	m_environmentPreFilterPassShader = &EnvironmentPreFilterPassPBSShader::getInstance();
	m_environmentBRDFLUTPassShader = &EnvironmentBRDFLUTPassPBSShader::getInstance();

	m_skyForwardPassShader = &SkyForwardPassPBSShader::getInstance();
	m_skyDeferPassShader = &SkyDeferPassPBSShader::getInstance();

	m_debuggerPassShader = &DebuggerShader::getInstance();

	m_finalPassShader = &FinalPassShader::getInstance();
}

void GLRenderingManager::initialize()
{
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

