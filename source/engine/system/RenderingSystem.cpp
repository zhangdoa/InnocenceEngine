#include "RenderingSystem.h"

GLShader::GLShader()
{
}

inline void GLShader::addShader(shaderType shaderType, const std::string & fileLocation) const
{
	attachShader(shaderType, g_pAssetSystem->loadShader(fileLocation), m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("Error: Attribute lost: " + arrtributeName);
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
		g_pLogSystem->printLog("Error: Uniform lost: " + uniform);
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

inline void GLShader::updateUniform(const GLint uniformLocation, double uniformValue) const
{
	glUniform1f(uniformLocation, uniformValue);
}

inline void GLShader::updateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, x, y);
}

inline void GLShader::updateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, x, y, z);
}

inline void GLShader::updateUniform(const GLint uniformLocation, double x, double y, double z, double w)
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
	default: g_pLogSystem->printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);

	if (l_shader == 0) {
		g_pLogSystem->printLog("Shader creation failed: memory location invaild when adding shader!");
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
		g_pLogSystem->printLog(&ShaderErrorMessage[0]);
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
	g_pLogSystem->printLog("Shader is compiled.");
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

void BillboardPassShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
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
			updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

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

void GeometryPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
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
			updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

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

void LightPassBlinnPhongShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode)
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

		updateUniform(m_uni_viewPos, cameraComponents[0]->getParentEntity()->getTransform()->getPos().x, cameraComponents[0]->getParentEntity()->getTransform()->getPos().y, cameraComponents[0]->getParentEntity()->getTransform()->getPos().z);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
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

void GeometryPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
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
			updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

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

	m_uni_textureMode = getUniformLocation("uni_textureMode");
	
	m_uni_shadingMode = getUniformLocation("uni_shadingMode");

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode)
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

		updateUniform(m_uni_viewPos, cameraComponents[0]->getParentEntity()->getTransform()->getPos().x, cameraComponents[0]->getParentEntity()->getTransform()->getPos().y, cameraComponents[0]->getParentEntity()->getTransform()->getPos().z);
		updateUniform(m_uni_textureMode, textureMode);
		updateUniform(m_uni_shadingMode, shadingMode);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform(m_uni_dirLight_direction, lightComponents[i]->getDirection().x, lightComponents[i]->getDirection().y, lightComponents[i]->getDirection().z);
			updateUniform(m_uni_dirLight_color, lightComponents[i]->getColor().x, lightComponents[i]->getColor().y, lightComponents[i]->getColor().z);
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			updateUniform(m_uni_pointLights_position[i + l_pointLightIndexOffset], lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
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

void EnvironmentCapturePassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);
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

void EnvironmentConvolutionPassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDConvolutedTexture)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);

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

void EnvironmentPreFilterPassPBSShader::shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDPreFiltedTexture)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);

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
					unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
					unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
					glViewport(0, 0, mipWidth, mipHeight);

					double roughness = (double)mip / (double)(maxMipLevels - 1);
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

void SkyForwardPassPBSShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDTexture)
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

void DebuggerShader::shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap)
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
			updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());

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

void RenderingSystem::setup()
{
	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_SAMPLES, 16); // 16x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef INNO_PLATFORM_MACOS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

	// Open a window and create its OpenGL context
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(m_window);
	if (m_window == nullptr) {
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLAD.");
	}
	
	// MSAA
	glEnable(GL_MULTISAMPLE);

	//setup input
	for (int i = 0; i < NUM_KEYCODES; i++)
	{
		m_keyButtonMap.emplace(i, keyButton());
	}
	f_changeDrawPolygonMode = std::bind(&RenderingSystem::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingSystem::changeDrawTextureMode, this);
	f_changeShadingMode = std::bind(&RenderingSystem::changeShadingMode, this);

	//setup rendering
	//@TODO: add a switch for different shader model
	//m_geometryPassShader = g_pMemorySystem->spawn<GeometryPassBlinnPhongShader>();
	m_geometryPassShader = g_pMemorySystem->spawn<GeometryPassPBSShader>();

	//m_lightPassShader = g_pMemorySystem->spawn<LightPassBlinnPhongShader>();
	m_lightPassShader = g_pMemorySystem->spawn<LightPassPBSShader>();

	m_environmentCapturePassShader = g_pMemorySystem->spawn<EnvironmentCapturePassPBSShader>();
	m_environmentConvolutionPassShader = g_pMemorySystem->spawn<EnvironmentConvolutionPassPBSShader>();
	m_environmentPreFilterPassShader = g_pMemorySystem->spawn<EnvironmentPreFilterPassPBSShader>();
	m_environmentBRDFLUTPassShader = g_pMemorySystem->spawn<EnvironmentBRDFLUTPassPBSShader>();

	m_skyForwardPassShader = g_pMemorySystem->spawn<SkyForwardPassPBSShader>();
	m_skyDeferPassShader = g_pMemorySystem->spawn<SkyDeferPassPBSShader>();

	m_debuggerPassShader = g_pMemorySystem->spawn<DebuggerShader>();

	m_finalPassShader = g_pMemorySystem->spawn<FinalPassShader>();

	m_objectStatus = objectStatus::ALIVE;
}

void RenderingSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::getInstance().setRenderingSystem(this);
	windowCallbackWrapper::getInstance().initialize();

	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);

	//initialize input
	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addKeyboardInputCallback(g_pGameSystem->getInputComponents()[i]->getKeyboardInputCallbackImpl());
		addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->getMouseInputCallbackImpl());
	}

	// @TODO: debt I owe
	addKeyboardInputCallback(GLFW_KEY_Q, &f_changeDrawPolygonMode);
	m_keyButtonMap.find(GLFW_KEY_Q)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);
	m_keyButtonMap.find(GLFW_KEY_E)->second.m_keyPressType = keyPressType::ONCE;
	addKeyboardInputCallback(GLFW_KEY_R, &f_changeShadingMode);
	m_keyButtonMap.find(GLFW_KEY_R)->second.m_keyPressType = keyPressType::ONCE;
	//initialize rendering
	glEnable(GL_TEXTURE_2D);
	initializeGeometryPass();
	initializeLightPass();
	initializeBackgroundPass();
	initializeFinalPass();

	//load assets
	m_basicNormalTemplate = addTexture(textureType::NORMAL);
	m_basicAlbedoTemplate = addTexture(textureType::ALBEDO);
	m_basicMetallicTemplate = addTexture(textureType::METALLIC);
	m_basicRoughnessTemplate = addTexture(textureType::ROUGHNESS);
	m_basicAOTemplate = addTexture(textureType::AMBIENT_OCCLUSION);

	g_pAssetSystem->loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicNormalTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicAlbedoTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_metallic.png" }, textureType::METALLIC, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicMetallicTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicRoughnessTemplate));
	g_pAssetSystem->loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, getTexture(textureType::NORMAL, m_basicAOTemplate));

	m_UnitQuadTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastQuadMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitQuadTemplate);
	lastQuadMeshData->addUnitQuad();
	lastQuadMeshData->setup(meshDrawMethod::TRIANGLE, true, true);
	lastQuadMeshData->initialize();

	m_UnitCubeTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastCubeMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitCubeTemplate);
	lastCubeMeshData->addUnitCube();
	lastCubeMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastCubeMeshData->initialize();

	m_UnitSphereTemplate = addMesh(meshType::THREE_DIMENSION);
	auto lastSphereMeshData = getMesh(meshType::THREE_DIMENSION, m_UnitSphereTemplate);
	lastSphereMeshData->addUnitSphere();
	lastSphereMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastSphereMeshData->initialize();

	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_meshType == meshShapeType::CUSTOM)
		{
			if (i->m_modelFileName != "")
			{
				loadModel(i->m_modelFileName, *i);
			}
		}
		else
		{
			assignUnitMesh(*i, i->m_meshType);
		}
		if (i->m_textureFileNameMap.size() != 0)
		{
			for (auto& j : i->m_textureFileNameMap)
			{
				loadTexture({ j.second }, j.first, *i);
			}
		}
	}
	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("RenderingSystem has been initialized.");
}

void RenderingSystem::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0)
	{
		glfwPollEvents();

		//Input update
		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			//if key pressed
			if (glfwGetKey(m_window, i) == GLFW_PRESS)
			{
				auto l_keyButton = m_keyButtonMap.find(i);
				if (l_keyButton != m_keyButtonMap.end())
				{
					//check whether it's still pressed/ the bound functions has been invoked
					if (l_keyButton->second.m_allowCallback)
					{
						auto l_keybinding = m_keyboardInputCallback.find(i);
						if (l_keybinding != m_keyboardInputCallback.end())
						{
							for (auto j : l_keybinding->second)
							{
								if (j)
								{
									(*j)();
								}
							}
						}
						if (l_keyButton->second.m_keyPressType == keyPressType::ONCE)
						{
							l_keyButton->second.m_allowCallback = false;
						}
					}

				}
			}
			else
			{
				auto l_keyButton = m_keyButtonMap.find(i);
				if (l_keyButton != m_keyButtonMap.end())
				{
					if (l_keyButton->second.m_keyPressType == keyPressType::ONCE)
					{
						l_keyButton->second.m_allowCallback = true;
					}
				}
			}
		}
		if (glfwGetMouseButton(m_window, 1) == GLFW_PRESS)
		{
			hideMouseCursor();
			if (m_mouseMovementCallback.size() != 0)
			{
				if (m_mouseXOffset != 0)
				{
					for (auto j : m_mouseMovementCallback.find(0)->second)
					{
						(*j)(m_mouseXOffset);
					};
				}
				if (m_mouseYOffset != 0)
				{
					for (auto j : m_mouseMovementCallback.find(1)->second)
					{
						(*j)(m_mouseYOffset);
					};
				}
				if (m_mouseXOffset != 0 || m_mouseYOffset != 0)
				{
					m_mouseXOffset = 0;
					m_mouseYOffset = 0;
				}
			}
		}
		else
		{
			showMouseCursor();
		}
	}
	else
	{
		g_pLogSystem->printLog("Input error!");
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void RenderingSystem::shutdown()
{
	if (m_window != nullptr)
	{
		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		g_pLogSystem->printLog("Window closed.");
	}
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("RenderingSystem has been shutdown.");
}

bool RenderingSystem::canRender()
{
	return m_canRender;
}

const objectStatus & RenderingSystem::getStatus() const
{
	return m_objectStatus;
}

GLFWwindow * RenderingSystem::getWindow() const
{
	return m_window;
}

vec2 RenderingSystem::getScreenCenterPosition() const
{
	return vec2(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
}

vec2 RenderingSystem::getScreenResolution() const
{
	return vec2(SCR_WIDTH, SCR_HEIGHT);
}

void RenderingSystem::setWindowName(const std::string & windowName)
{
	m_windowName = windowName;
}

void RenderingSystem::hideMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void RenderingSystem::showMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void RenderingSystem::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
{
	auto l_keyboardInputCallbackFunctionVector = m_keyboardInputCallback.find(keyCode);
	if (l_keyboardInputCallbackFunctionVector != m_keyboardInputCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(keyboardInputCallback);
	}
	else
	{
		m_keyboardInputCallback.emplace(keyCode, std::vector<std::function<void()>*>{keyboardInputCallback});
	}
}

void RenderingSystem::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void RenderingSystem::addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void RenderingSystem::addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = m_mouseMovementCallback.find(keyCode);
	if (l_mouseMovementCallbackFunctionVector != m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallback.emplace(keyCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void RenderingSystem::addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(keyCode, i);
	}
}

void RenderingSystem::addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void windowCallbackWrapper::setRenderingSystem(RenderingSystem * RenderingSystem)
{
	m_renderingSystem = RenderingSystem;
}

void windowCallbackWrapper::initialize()
{
	glfwSetFramebufferSizeCallback(m_renderingSystem->m_window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(m_renderingSystem->m_window, &mousePositionCallback);
	glfwSetScrollCallback(m_renderingSystem->m_window, &scrollCallback);
}

void windowCallbackWrapper::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void windowCallbackWrapper::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	getInstance().scrollCallbackImpl(window, xoffset, yoffset);
}

void windowCallbackWrapper::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	m_renderingSystem->SCR_WIDTH = width;
	m_renderingSystem->SCR_HEIGHT = height;
	glViewport(0, 0, width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_renderingSystem->m_mouseXOffset = mouseXPos - m_renderingSystem->m_mouseLastX;
	m_renderingSystem->m_mouseYOffset = m_renderingSystem->m_mouseLastY - mouseYPos;

	m_renderingSystem->m_mouseLastX = mouseXPos;
	m_renderingSystem->m_mouseLastY = mouseYPos;
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
}

meshID RenderingSystem::addMesh(meshType meshType)
{
	if (meshType == meshType::TWO_DIMENSION)
	{
		GL2DMesh newMesh;
		m_2DMeshMap.emplace(std::pair<meshID, GL2DMesh>(newMesh.getMeshID(), newMesh));
		return newMesh.getMeshID();
	}
	else if (meshType == meshType::THREE_DIMENSION)
	{
		GL3DMesh newMesh;
		m_3DMeshMap.emplace(std::pair<meshID, GL3DMesh>(newMesh.getMeshID(), newMesh));
		return newMesh.getMeshID();
	}
}

textureID RenderingSystem::addTexture(textureType textureType)
{
	if (textureType == textureType::CUBEMAP)
	{
		GL3DTexture new3DTexture;
		m_3DTextureMap.emplace(std::pair<textureID, GL3DTexture>(new3DTexture.getTextureID(), new3DTexture));
		return new3DTexture.getTextureID();
	}
	else if (textureType == textureType::CUBEMAP_HDR)
	{
		GL3DHDRTexture new3DHDRTexture;
		m_3DHDRTextureMap.emplace(std::pair<textureID, GL3DHDRTexture>(new3DHDRTexture.getTextureID(), new3DHDRTexture));
		return new3DHDRTexture.getTextureID();
	}
	else if (textureType == textureType::EQUIRETANGULAR)
	{
		GL2DHDRTexture new2DHDRTexture;
		m_2DHDRTextureMap.emplace(std::pair<textureID, GL2DHDRTexture>(new2DHDRTexture.getTextureID(), new2DHDRTexture));
		return new2DHDRTexture.getTextureID();
	}
	else
	{
		GL2DTexture new2DTexture;
		m_2DTextureMap.emplace(std::pair<textureID, GL2DTexture>(new2DTexture.getTextureID(), new2DTexture));
		return new2DTexture.getTextureID();
	}
}

BaseMesh* RenderingSystem::getMesh(meshType meshType, meshID meshID)
{
	if (meshType == meshType::TWO_DIMENSION)
	{
		return &m_2DMeshMap.find(meshID)->second;
	}
	else if (meshType == meshType::THREE_DIMENSION)
	{
		return &m_3DMeshMap.find(meshID)->second;
	}

}

BaseTexture * RenderingSystem::getTexture(textureType textureType, textureID textureID)
{
	if (textureType == textureType::CUBEMAP)
	{
		return &m_3DTextureMap.find(textureID)->second;
	}
	else if (textureType == textureType::CUBEMAP_HDR)
	{
		return &m_3DHDRTextureMap.find(textureID)->second;
	}
	else if (textureType == textureType::EQUIRETANGULAR)
	{
		return &m_2DHDRTextureMap.find(textureID)->second;
	}
	else
	{
		return &m_2DTextureMap.find(textureID)->second;
	}
}

void RenderingSystem::assignUnitMesh(VisibleComponent & visibleComponent, meshShapeType meshType)
{
	meshID l_UnitMeshTemplate;
	switch (meshType)
	{
	case meshShapeType::QUAD: l_UnitMeshTemplate = m_UnitQuadTemplate; break;
	case meshShapeType::CUBE: l_UnitMeshTemplate = m_UnitCubeTemplate; break;
	case meshShapeType::SPHERE: l_UnitMeshTemplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);
	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
}

void RenderingSystem::assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD_DEFAULT)
	{
		visibleComponent.addTextureData(loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedtexturePair);
	}
}

void RenderingSystem::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, m_basicAOTemplate), visibleComponent);
	}
}

void RenderingSystem::assignloadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
}

void RenderingSystem::loadTexture(const std::vector<std::string> &fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	for (auto& i : fileName)
	{
		auto l_loadedTexturePair = m_loadedTextureMap.find(i);
		// check if this file has already loaded
		if (l_loadedTexturePair != m_loadedTextureMap.end())
		{
			assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTexturePair->second, visibleComponent);
			g_pLogSystem->printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_textureID = addTexture(textureType);
			auto l_baseTexture = getTexture(textureType, l_textureID);
			g_pAssetSystem->loadTextureFromDisk({ i }, textureType, visibleComponent.m_textureWrapMethod, l_baseTexture);
			m_loadedTextureMap.emplace(i, texturePair(textureType, l_textureID));
			assignLoadedTexture(textureAssignType::OVERWRITE, texturePair(textureType, l_textureID), visibleComponent);
		}
	}

}

void RenderingSystem::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedmodelMap->second, visibleComponent);
		g_pLogSystem->printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		modelMap l_modelMap;
		g_pAssetSystem->loadModelFromDisk(fileName, l_modelMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod);

		//mark as loaded
		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
		assignloadedModel(l_modelMap, visibleComponent);
	}
}

void RenderingSystem::render()
{
	//defer render
	m_canRender = false;
	renderBackgroundPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderGeometryPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderLightPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	renderFinalPass(g_pGameSystem->getCameraComponents(), g_pGameSystem->getLightComponents(), g_pGameSystem->getVisibleComponents());
	//swap framebuffers
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0)
	{
		glfwSwapBuffers(m_window);
		m_canRender = true;
	}
	else
	{
		g_pLogSystem->printLog("Window error!");
		g_pLogSystem->printLog("RenderingSystem is stand-by.");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void RenderingSystem::initializeGeometryPass()
{
	// initialize shader
	m_geometryPassShader->init();
	m_geometryPassFrameBuffer.setup(m_screenResolution, frameBufferType::FORWARD, renderBufferType::DEPTH_AND_STENCIL, 4);
	m_geometryPassFrameBuffer.initialize();
}

void RenderingSystem::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	m_geometryPassFrameBuffer.update();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL - m_polygonMode);
	m_geometryPassShader->shaderDraw(cameraComponents, visibleComponents, m_3DMeshMap, m_2DTextureMap);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderingSystem::initializeBackgroundPass()
{
	// environment capture pass
	m_environmentCapturePassShader->init();

	glGenFramebuffers(1, &m_environmentPassFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_environmentPassFBO);

	glGenRenderbuffers(1, &m_environmentPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_environmentPassRBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_environmentPassRBO);

	// @TODO: add a capturer class
	m_environmentCapturePassTextureID = this->addTexture(textureType::CUBEMAP_HDR);
	auto l_environmentCapturePassTextureData = this->getTexture(textureType::CUBEMAP_HDR, m_environmentCapturePassTextureID);
	l_environmentCapturePassTextureData->setup(textureType::CUBEMAP_HDR, textureInternalFormat::RGB, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 2048, 2048, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
	l_environmentCapturePassTextureData->initialize();

	// environment convolution pass
	m_environmentConvolutionPassShader->init();

	m_environmentConvolutionPassTextureID = this->addTexture(textureType::CUBEMAP_HDR);
	auto l_environmentConvolutionPassTextureData = this->getTexture(textureType::CUBEMAP_HDR, m_environmentConvolutionPassTextureID);
	l_environmentConvolutionPassTextureData->setup(textureType::CUBEMAP_HDR, textureInternalFormat::RGB, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::LINEAR, textureFilterMethod::LINEAR, 128, 128, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
	l_environmentConvolutionPassTextureData->initialize();

	// environment pre-filter pass
	m_environmentPreFilterPassShader->init();

	m_environmentPreFilterPassTextureID = this->addTexture(textureType::CUBEMAP_HDR);
	auto l_environmentPreFilterPassTextureData = this->getTexture(textureType::CUBEMAP_HDR, m_environmentPreFilterPassTextureID);
	l_environmentPreFilterPassTextureData->setup(textureType::CUBEMAP_HDR, textureInternalFormat::RGB, textureWrapMethod::CLAMP_TO_EDGE, textureFilterMethod::LINEAR_MIPMAP_LINEAR, textureFilterMethod::LINEAR, 128, 128, std::vector<void*>{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
	l_environmentPreFilterPassTextureData->initialize();

	// environment brdf LUT pass
	m_environmentBRDFLUTPassShader->init();

	glGenTextures(1, &m_environmentBRDFLUTTexture);
	glBindTexture(GL_TEXTURE_2D, m_environmentBRDFLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// initialize background defer pass mesh
	m_environmentBRDFLUTMesh = this->getMesh(meshType::TWO_DIMENSION, this->addMesh(meshType::TWO_DIMENSION));
	m_environmentBRDFLUTMesh->addUnitQuad();
	m_environmentBRDFLUTMesh->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	m_environmentBRDFLUTMesh->initialize();

	// background forward pass
	m_skyForwardPassShader->init();
	m_skyForwardPassFrameBuffer.setup(m_screenResolution, frameBufferType::FORWARD, renderBufferType::DEPTH_AND_STENCIL, 1);
	m_skyForwardPassFrameBuffer.initialize();

	// initialize Debugger Pass shader
	m_debuggerPassShader->init();
	m_debuggerPassFrameBuffer.setup(m_screenResolution, frameBufferType::DEFER, renderBufferType::DEPTH_AND_STENCIL, 1);
	m_debuggerPassFrameBuffer.initialize();

	// background defer pass
	m_skyDeferPassShader->init();
	m_skyDeferPassFrameBuffer.setup(m_screenResolution, frameBufferType::DEFER, renderBufferType::DEPTH_AND_STENCIL, 1);
	m_skyDeferPassFrameBuffer.initialize();
}

void RenderingSystem::renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
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

		m_environmentCapturePassShader->shaderDraw(visibleComponents, m_3DMeshMap, m_2DHDRTextureMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second);

		// draw environment map convolution pass
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
		glViewport(0, 0, 128, 128);

		m_environmentConvolutionPassShader->shaderDraw(visibleComponents, m_3DMeshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second, m_3DHDRTextureMap.find(m_environmentConvolutionPassTextureID)->second);

		// draw environment map pre-filter pass
		m_environmentPreFilterPassShader->shaderDraw(visibleComponents, m_3DMeshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second, m_3DHDRTextureMap.find(m_environmentPreFilterPassTextureID)->second);

		// draw environment map BRDF LUT pass
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glViewport(0, 0, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_environmentBRDFLUTTexture, 0);

		m_environmentBRDFLUTPassShader->shaderDraw();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw environment map BRDF LUT rectangle
		m_environmentBRDFLUTMesh->update();
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (int)m_screenResolution.x, (int)m_screenResolution.x);
		glViewport(0, 0, (int)m_screenResolution.x, (int)m_screenResolution.y);

		m_shouldUpdateEnvironmentMap = false;
	}

	// draw background forward pass
	m_skyForwardPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_skyForwardPassShader->shaderDraw(cameraComponents, visibleComponents, m_3DMeshMap, m_3DHDRTextureMap.find(m_environmentCapturePassTextureID)->second);

	// draw debugger pass
	m_debuggerPassFrameBuffer.update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	m_debuggerPassShader->shaderDraw(cameraComponents, visibleComponents, m_3DMeshMap);

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

void RenderingSystem::initializeLightPass()
{
	// initialize shader
	m_lightPassShader->init();
	m_lightPassFrameBuffer.setup(m_screenResolution, frameBufferType::DEFER, renderBufferType::DEPTH_AND_STENCIL, 1);
	m_lightPassFrameBuffer.initialize();
}

void RenderingSystem::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
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

	m_lightPassShader->shaderDraw(cameraComponents, lightComponents, m_textureMode, m_shadingMode);

	// draw light pass rectangle
	m_lightPassFrameBuffer.drawMesh();
}

void RenderingSystem::initializeFinalPass()
{
	// initialize Final Pass shader
	m_finalPassShader->init();
	// initialize final pass mesh
	m_finalPassMesh = this->getMesh(meshType::TWO_DIMENSION, this->addMesh(meshType::TWO_DIMENSION));
	m_finalPassMesh->addUnitQuad();
	m_finalPassMesh->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	m_finalPassMesh->initialize();
}

void RenderingSystem::renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
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
	m_finalPassMesh->update();
}

void RenderingSystem::changeDrawPolygonMode()
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

void RenderingSystem::changeDrawTextureMode()
{
	if (m_textureMode == 4)
	{
		m_textureMode = 0;
	}
	else
	{
		m_textureMode += 1;
	}
}

void RenderingSystem::changeShadingMode()
{
	if (m_shadingMode == 1)
	{
		m_shadingMode = 0;
	}
	else
	{
		m_shadingMode += 1;
	}
}
