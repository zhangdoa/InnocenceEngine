#include "GLShader.h"

GLShader::GLShader()
{
}

GLShader::~GLShader()
{
}

void GLShader::setup()
{
}

void GLShader::setup(shaderType shaderType, const std::string& shaderFilePath, const std::vector<std::string>& attributions)
{
	m_shaderType = shaderType;
	m_shaderFilePath = shaderFilePath;
	m_attributions = attributions;

	m_objectStatus = objectStatus::ALIVE;
}

void GLShader::initialize()
{
	// @TODO: shouldn't we try to avoid to use this kind of ugly coupling impl???
	m_shaderCode = g_pAssetSystem->loadShader(m_shaderFilePath);

	int l_glShaderType = 0;

	switch (m_shaderType)
	{
	case shaderType::VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case shaderType::GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case shaderType::FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: g_pLogSystem->printLog("Error: Unknown shader type, cannot create shader!"); m_objectStatus = objectStatus::STANDBY; break;
	}

	m_shaderID = glCreateShader(l_glShaderType);

	if (m_shaderID == 0) {
		g_pLogSystem->printLog("Error: Shader creation failed: memory location invaild when adding shader!");
		m_objectStatus = objectStatus::STANDBY;
	}

	char const * l_sourcePointer = m_shaderCode.c_str();
	glShaderSource(m_shaderID, 1, &l_sourcePointer, NULL);
}

void GLShader::update()
{
}

void GLShader::shutdown()
{
	glDeleteShader(m_shaderID);
	m_objectStatus = objectStatus::SHUTDOWN;
}

const objectStatus & GLShader::getStatus() const
{
	return m_objectStatus;
}

const GLint & GLShader::getShaderID() const
{
	return m_shaderID;
}

const std::string & GLShader::getShaderFilePath() const
{
	return m_shaderFilePath;
}

const std::vector<std::string> & GLShader::getAttributions() const
{
	return m_attributions;
}

void GLShader::parseAttribution()
{
}

GLShaderProgram::GLShaderProgram()
{
}

GLShaderProgram::~GLShaderProgram()
{
}

void GLShaderProgram::setup()
{
}

void GLShaderProgram::setup(shaderTuple GLShaders)
{
	for (auto& i : GLShaders)
	{
		if (std::get<0>(i) == shaderType::VERTEX)
		{
			m_vertexShader.setup(std::get<0>(i), std::get<1>(i), std::get<2>(i));
		}
		else if (std::get<0>(i) == shaderType::GEOMETRY)
		{
			m_geometryShader.setup(std::get<0>(i), std::get<1>(i), std::get<2>(i));
		}
		else if (std::get<0>(i) == shaderType::FRAGMENT)
		{
			m_fragmentShader.setup(std::get<0>(i), std::get<1>(i), std::get<2>(i));
		}
	}
}

void GLShaderProgram::initialize()
{
	m_program = glCreateProgram();

	if (m_vertexShader.getStatus() == objectStatus::ALIVE)
	{
		m_vertexShader.initialize();
		attachShader(&m_vertexShader);
		for (unsigned int i = 0; i < m_vertexShader.getAttributions().size(); i++)
		{
			setAttributeLocation(i, m_vertexShader.getAttributions()[i]);
		}
	}
	if (m_geometryShader.getStatus() == objectStatus::ALIVE)
	{
		m_geometryShader.initialize();
		attachShader(&m_geometryShader);
	}
	if (m_fragmentShader.getStatus() == objectStatus::ALIVE)
	{
		m_fragmentShader.initialize();
		attachShader(&m_fragmentShader);
	}
}

void GLShaderProgram::shutdown()
{
	if (m_vertexShader.getStatus() == objectStatus::ALIVE)
	{
		m_vertexShader.shutdown();
		glDetachShader(m_program, m_vertexShader.getShaderID());
	}
	if (m_geometryShader.getStatus() == objectStatus::ALIVE)
	{
		m_geometryShader.shutdown();
		glDetachShader(m_program, m_geometryShader.getShaderID());
	}
	if (m_fragmentShader.getStatus() == objectStatus::ALIVE)
	{
		m_fragmentShader.shutdown();
		glDetachShader(m_program, m_fragmentShader.getShaderID());
	}
}

void GLShaderProgram::attachShader(const GLShader* GLShader) const
{
	GLint Result = GL_FALSE;
	int l_infoLogLength = 0;

	glGetShaderiv(m_program, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(m_program, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (l_infoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(m_program, l_infoLogLength, NULL, &ShaderErrorMessage[0]);
		g_pLogSystem->printLog(&ShaderErrorMessage[0]);
	}

	const GLint l_shaderID = GLShader->getShaderID();

	glAttachShader(m_program, l_shaderID);
	glLinkProgram(m_program);
	glValidateProgram(m_program);

	g_pLogSystem->printLog("innoShader: " + GLShader->getShaderFilePath() + " Shader is compiled.");

	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shaderID, 1024, NULL, infoLog);
		g_pLogSystem->printLog("innoShader: Error: Shader compile error: " + std::string(infoLog) + "\n -- --------------------------------------------------- -- ");
	}
}

void GLShaderProgram::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Attribute lost: " + arrtributeName);
	}
}

inline void GLShaderProgram::useProgram() const
{
	glUseProgram(m_program);
}

inline void GLShaderProgram::addUniform(std::string uniform) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Uniform lost: " + uniform);
	}
}

inline GLint GLShaderProgram::getUniformLocation(const std::string & uniformName) const
{
	return glGetUniformLocation(m_program, uniformName.c_str());
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, bool uniformValue) const
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, int uniformValue) const
{
	glUniform1i(uniformLocation, uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double uniformValue) const
{
	glUniform1f(uniformLocation, uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, x, y);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, x, y, z);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z, double w)
{
	glUniform4f(uniformLocation, x, y, z, w);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, const mat4 & mat) const
{
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
}

void BillboardPassShaderProgram::initialize()
{
	this->setup({std::make_tuple(shaderType::VERTEX, std::string("GL3.3/billboardPassVertex.sf"), std::vector<std::string>{ "in_Position" , "in_TexCoord" }), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/billboardPassFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_texture = getUniformLocation("uni_texture");
	updateUniform(m_uni_texture, 0);
	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void BillboardPassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	useProgram();

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

void GeometryPassBlinnPhongShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/geometryPassBlinnPhongVertex.sf"), std::vector<std::string>{ "in_Position" , "in_TexCoord",  "in_Normal"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/geometryPassBlinnPhongFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

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

void GeometryPassBlinnPhongShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	useProgram();

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

void LightPassBlinnPhongShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/lightPassBlinnPhongVertex.sf"), std::vector<std::string>{ "in_Position" , "in_TexCoord"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/lightPassBlinnPhongFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

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

void LightPassBlinnPhongShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode)
{
	useProgram();

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

void GeometryPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/geometryPassPBSVertex.sf"), std::vector<std::string>{ "in_Position" , "in_TexCoord",  "in_Normal"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/geometryPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

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

void GeometryPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap)
{
	useProgram();

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

void LightPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/lightPassPBSVertex.sf"), std::vector<std::string>{ "in_Position" , "in_TexCoord"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/lightPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

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

void LightPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode)
{
	useProgram();

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

void EnvironmentCapturePassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/environmentCapturePassPBSVertex.sf"), std::vector<std::string>{ "in_Position"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/environmentCapturePassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_equirectangularMap = getUniformLocation("uni_equirectangularMap");
	updateUniform(m_uni_equirectangularMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentCapturePassPBSShaderProgram::update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture)
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

	useProgram();
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

void EnvironmentConvolutionPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/environmentConvolutionPassPBSVertex.sf"), std::vector<std::string>{ "in_Position"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/environmentConvolutionPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentConvolutionPassPBSShaderProgram::update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDConvolutedTexture)
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

	useProgram();
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

void EnvironmentPreFilterPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/environmentPreFilterPassPBSVertex.sf"), std::vector<std::string>{ "in_Position"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/environmentPreFilterPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");

	m_uni_roughness = getUniformLocation("uni_roughness");
}

void EnvironmentPreFilterPassPBSShaderProgram::update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture & threeDCapturedTexture, GL3DHDRTexture & threeDPreFiltedTexture)
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

	useProgram();
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

void EnvironmentBRDFLUTPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/environmentBRDFLUTPassPBSVertex.sf"), std::vector<std::string>{ "in_Position", "in_TexCoord"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/environmentBRDFLUTPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();
}

void EnvironmentBRDFLUTPassPBSShaderProgram::update()
{
	useProgram();
}

void SkyForwardPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/skyForwardPassPBSVertex.sf"), std::vector<std::string>{ "in_Position"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/skyForwardPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_skybox = getUniformLocation("uni_skybox");
	updateUniform(m_uni_skybox, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void SkyForwardPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDTexture)
{
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	useProgram();

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

void DebuggerShaderProgram::initialize()
{
	this->setup({	std::make_tuple(shaderType::VERTEX, std::string("GL3.3/debuggerVertex.sf"), std::vector<std::string>{ "in_Position", "in_TexCoord", "in_Normal"}),
					std::make_tuple(shaderType::GEOMETRY, std::string("GL3.3/debuggerGeometry.sf"), std::vector<std::string>{}),
					std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/debuggerFragment.sf"), std::vector<std::string>{}),
				});
	GLShaderProgram::initialize();
	useProgram();

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void DebuggerShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap)
{
	useProgram();

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


void SkyDeferPassPBSShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/skyDeferPassPBSVertex.sf"), std::vector<std::string>{ "in_Position",  "in_TexCoord"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/skyDeferPassPBSFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_lightPassRT0 = getUniformLocation("uni_lightPassRT0");
	updateUniform(m_uni_lightPassRT0, 0);
	m_uni_skyForwardPassRT0 = getUniformLocation("uni_skyForwardPassRT0");
	updateUniform(m_uni_skyForwardPassRT0, 1);
	m_uni_debuggerPassRT0 = getUniformLocation("uni_debuggerPassRT0");
	updateUniform(m_uni_debuggerPassRT0, 2);
}

void SkyDeferPassPBSShaderProgram::update()
{
	useProgram();
}


void FinalPassShaderProgram::initialize()
{
	this->setup({ std::make_tuple(shaderType::VERTEX, std::string("GL3.3/finalPassVertex.sf"), std::vector<std::string>{ "in_Position",  "in_TexCoord"}), std::make_tuple(shaderType::FRAGMENT, std::string("GL3.3/finalPassFragment.sf"), std::vector<std::string>{}) });
	GLShaderProgram::initialize();
	useProgram();

	m_uni_skyDeferPassRT0 = getUniformLocation("uni_skyDeferPassRT0");
	updateUniform(m_uni_skyDeferPassRT0, 0);
}

void FinalPassShaderProgram::update()
{
	useProgram();
}
