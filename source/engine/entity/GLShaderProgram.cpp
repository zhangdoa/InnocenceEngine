#include "GLShaderProgram.h"

GLShaderProgram::GLShaderProgram()
{
}

GLShaderProgram::~GLShaderProgram()
{
}

void GLShaderProgram::initialize()
{
	m_program = glCreateProgram();

	for (auto& i : m_shaderDatas)
	{
		if (std::get<shaderType>(i) == shaderType::VERTEX)
		{
			m_vertexShader = g_pMemorySystem->spawn<GLShader>();
			m_vertexShader->setup(i);
			if (m_vertexShader->getStatus() == objectStatus::ALIVE)
			{
				m_vertexShader->initialize();
				attachShader(m_vertexShader);
			}
		}
		else if (std::get<shaderType>(i) == shaderType::GEOMETRY)
		{
			m_geometryShader = g_pMemorySystem->spawn<GLShader>();
			m_geometryShader->setup(i);

			if (m_geometryShader->getStatus() == objectStatus::ALIVE)
			{
				m_geometryShader->initialize();
				attachShader(m_geometryShader);
			}
		}
		else if (std::get<shaderType>(i) == shaderType::FRAGMENT)
		{
			m_fragmentShader = g_pMemorySystem->spawn<GLShader>();
			m_fragmentShader->setup(i);
			if (m_fragmentShader->getStatus() == objectStatus::ALIVE)
			{
				m_fragmentShader->initialize();
				attachShader(m_fragmentShader);
			}
		}
	}
}

void GLShaderProgram::shutdown()
{
	if (m_vertexShader->getStatus() == objectStatus::ALIVE)
	{
		m_vertexShader->shutdown();
		glDetachShader(m_program, m_vertexShader->getShaderID());
	}
	if (m_geometryShader->getStatus() == objectStatus::ALIVE)
	{
		m_geometryShader->shutdown();
		glDetachShader(m_program, m_geometryShader->getShaderID());
	}
	if (m_fragmentShader->getStatus() == objectStatus::ALIVE)
	{
		m_fragmentShader->shutdown();
		glDetachShader(m_program, m_fragmentShader->getShaderID());
	}
}

void GLShaderProgram::attachShader(GLShader* GLShader) const
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

	g_pLogSystem->printLog("innoShader: " + std::get<shaderCodeContentPair>(GLShader->getShaderData()).first + " Shader is compiled.");

	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shaderID, 1024, NULL, infoLog);
		g_pLogSystem->printLog("innoShader: " + std::get<shaderCodeContentPair>(GLShader->getShaderData()).first + " compile error: " + std::string(infoLog) + "\n -- --------------------------------------------------- -- ");
	}
}

inline void GLShaderProgram::useProgram() const
{
	glUseProgram(m_program);
}


inline GLint GLShaderProgram::getUniformLocation(const std::string & uniformName) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
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
	glUniform1f(uniformLocation, (GLfloat)uniformValue);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, (GLfloat)x, (GLfloat)y);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, double x, double y, double z, double w) const
{
	glUniform4f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

inline void GLShaderProgram::updateUniform(const GLint uniformLocation, const mat4 & mat) const
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m[0][0]);
#endif
}

void ShadowForwardPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_p = getUniformLocation("uni_p");
	m_uni_v = getUniformLocation("uni_v");
	m_uni_m = getUniformLocation("uni_m");
}

void ShadowForwardPassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);
	useProgram();

	// draw each lightComponent's shadowmap
	for (auto& l_lightComponent : lightComponents)
	{
		if (l_lightComponent->getLightType() == lightType::DIRECTIONAL)
		{
			auto l_boundMax = l_lightComponent->m_AABB.m_boundMax;
			auto l_boundMin = l_lightComponent->m_AABB.m_boundMin;
			auto l_pos = l_lightComponent->m_AABB.m_center;
			mat4 p_light;
			p_light.initializeToOrthographicMatrix(l_boundMin.x, l_boundMax.x, l_boundMin.y, l_boundMax.y, 0.0, l_boundMax.z - l_boundMin.z);
			mat4 v = l_pos.toTranslationMatrix() * l_lightComponent->getInvertTranslationMatrix() * l_lightComponent->getInvertRotationMatrix();
			updateUniform(m_uni_p, p_light);
			updateUniform(m_uni_v, v);

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
						meshMap.find(l_graphicData.first)->second->update();
					}
				}
			}
		}
	}
}

void ShadowDeferPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_shadowForwardPassRT0 = getUniformLocation("uni_shadowForwardPassRT0");
	updateUniform(m_uni_shadowForwardPassRT0, 0);
}

void ShadowDeferPassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	useProgram();
}

void GeometryPassBlinnPhongShaderProgram::initialize()
{
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

void GeometryPassBlinnPhongShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	// @TODO
	glDisable(GL_CULL_FACE);
	useProgram();

	mat4 p = cameraComponents[0]->getProjectionMatrix();
	mat4 r = cameraComponents[0]->getInvertRotationMatrix();
	mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

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
						l_textureData->update(0);
					}
					// any diffuse?
					auto l_diffuseTextureID = l_textureMap.find(textureType::ALBEDO);
					if (l_diffuseTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
						l_textureData->update(1);
					}
					// any specular?
					auto l_specularTextureID = l_textureMap.find(textureType::METALLIC);
					if (l_specularTextureID != l_textureMap.end())
					{
						auto& l_textureData = textureMap.find(l_specularTextureID->second)->second;
						l_textureData->update(2);
					}
				}
				// draw meshes
				meshMap.find(l_graphicData.first)->second->update();
			}
		}
	}
}

void LightPassBlinnPhongShaderProgram::initialize()
{
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

void LightPassBlinnPhongShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
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
	m_uni_p_light = getUniformLocation("uni_p_light");
	m_uni_v_light = getUniformLocation("uni_v_light");

	m_uni_useTexture = getUniformLocation("uni_useTexture");
	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_MRA = getUniformLocation("uni_MRA");
}

void GeometryPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	// @TODO
	glDisable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL - m_polygonMode);

	useProgram();

	if (cameraComponents.size() > 0)
	{
		mat4 p = cameraComponents[0]->getProjectionMatrix();
		mat4 r = cameraComponents[0]->getInvertRotationMatrix();
		mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

		updateUniform(m_uni_p, p);
		updateUniform(m_uni_r, r);
		updateUniform(m_uni_t, t);
	}

	if (lightComponents.size() > 0)
	{
		for (auto& l_lightComponent : lightComponents)
		{
			// update light space transformation matrices
			if (l_lightComponent->getLightType() == lightType::DIRECTIONAL)
			{
				auto l_boundMax = l_lightComponent->m_AABB.m_boundMax;
				auto l_boundMin = l_lightComponent->m_AABB.m_boundMin;
				auto l_pos = l_lightComponent->m_AABB.m_center;
				mat4 p_light;
				p_light.initializeToOrthographicMatrix(l_boundMin.x, l_boundMax.x, l_boundMin.y, l_boundMax.y, 0.0, l_boundMax.z - l_boundMin.z);
				mat4 v = l_pos.toTranslationMatrix() * l_lightComponent->getInvertTranslationMatrix() * l_lightComponent->getInvertRotationMatrix();

				updateUniform(m_uni_p_light, p_light);
				updateUniform(m_uni_v_light, v);

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
									l_textureData->update(0);
								}
								// any albedo?
								auto& l_albedoTextureID = l_textureMap.find(textureType::ALBEDO);
								if (l_albedoTextureID != l_textureMap.end())
								{
									auto& l_textureData = textureMap.find(l_albedoTextureID->second)->second;
									l_textureData->update(1);
								}
								// any metallic?
								auto& l_metallicTextureID = l_textureMap.find(textureType::METALLIC);
								if (l_metallicTextureID != l_textureMap.end())
								{
									auto& l_textureData = textureMap.find(l_metallicTextureID->second)->second;
									l_textureData->update(2);
								}
								// any roughness?
								auto& l_roughnessTextureID = l_textureMap.find(textureType::ROUGHNESS);
								if (l_roughnessTextureID != l_textureMap.end())
								{
									auto& l_textureData = textureMap.find(l_roughnessTextureID->second)->second;
									l_textureData->update(3);
								}
								// any ao?
								auto& l_aoTextureID = l_textureMap.find(textureType::AMBIENT_OCCLUSION);
								if (l_aoTextureID != l_textureMap.end())
								{
									auto& l_textureData = textureMap.find(l_aoTextureID->second)->second;
									l_textureData->update(4);
								}
							}
							updateUniform(m_uni_useTexture, l_visibleComponent->m_useTexture);
							updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
							updateUniform(m_uni_MRA, l_visibleComponent->m_MRA.x, l_visibleComponent->m_MRA.y, l_visibleComponent->m_MRA.z);
							// draw meshes
							meshMap.find(l_graphicData.first)->second->update();
						}
					}
				}
			}
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void LightPassPBSShaderProgram::initialize()
{
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
	m_uni_geometryPassRT4 = getUniformLocation("uni_geometryPassRT4");
	updateUniform(m_uni_geometryPassRT4, 4);
	m_uni_shadowMap = getUniformLocation("uni_shadowMap");
	updateUniform(m_uni_shadowMap, 5);
	m_uni_irradianceMap = getUniformLocation("uni_irradianceMap");
	updateUniform(m_uni_irradianceMap, 6);
	m_uni_preFiltedMap = getUniformLocation("uni_preFiltedMap");
	updateUniform(m_uni_preFiltedMap, 7);
	m_uni_brdfLUT = getUniformLocation("uni_brdfLUT");
	updateUniform(m_uni_brdfLUT, 8);

	m_uni_textureMode = getUniformLocation("uni_textureMode");

	m_uni_shadingMode = getUniformLocation("uni_shadingMode");

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_position = getUniformLocation("uni_dirLight.position");
	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	useProgram();

	if (lightComponents.size() > 0)
	{
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
			//updateUniform(m_uni_textureMode, textureMode);
			//updateUniform(m_uni_shadingMode, shadingMode);

			if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
			{
				l_pointLightIndexOffset -= 1;
				updateUniform(m_uni_dirLight_position, lightComponents[i]->getParentEntity()->getTransform()->getPos().x, lightComponents[i]->getParentEntity()->getTransform()->getPos().y, lightComponents[i]->getParentEntity()->getTransform()->getPos().z);
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
}

void EnvironmentCapturePassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_equirectangularMap = getUniformLocation("uni_equirectangularMap");
	updateUniform(m_uni_equirectangularMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentCapturePassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);
	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(-1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	useProgram();
	updateUniform(m_uni_p, captureProjection);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{

				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					// activate equiretangular texture and remapping equiretangular texture to cubemap
					auto l_equiretangularTexture = textureMap.find(l_graphicData.second.find(textureType::EQUIRETANGULAR)->second);
					auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
					if (l_equiretangularTexture != textureMap.end() && l_environmentCaptureTexture != textureMap.end())
					{
						l_equiretangularTexture->second->update(0);
						for (unsigned int i = 0; i < 6; ++i)
						{
							updateUniform(m_uni_r, captureViews[i]);
							l_environmentCaptureTexture->second->attachToFramebuffer(0, i, 0);
							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							meshMap.find(l_graphicData.first)->second->update();
						}
						l_environmentCaptureTexture->second->update(0);
						glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
					}
				}
			}
		}
	}
}

void EnvironmentConvolutionPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentConvolutionPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);

	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(-1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	useProgram();
	updateUniform(m_uni_p, captureProjection);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
					auto l_environmentConvolutionTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CONVOLUTION)->second);
					if (l_environmentCaptureTexture != textureMap.end() && l_environmentConvolutionTexture != textureMap.end())
					{
						// @TODO: it should be update(0)?
						l_environmentCaptureTexture->second->update(1);
						for (unsigned int i = 0; i < 6; ++i)
						{
							updateUniform(m_uni_r, captureViews[i]);
							l_environmentConvolutionTexture->second->attachToFramebuffer(0, i, 0);
							glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
							meshMap.find(l_graphicData.first)->second->update();
						}
					}
				}
			}
		}
	}
}

void EnvironmentPreFilterPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	updateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");

	m_uni_roughness = getUniformLocation("uni_roughness");
}

void EnvironmentPreFilterPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	mat4 captureProjection;
	captureProjection.initializeToPerspectiveMatrix((90.0 / 180.0) * PI, 1.0f, 0.1f, 10.0f);

	std::vector<mat4> captureViews =
	{
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(-1.0f,  0.0f,  0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f,  1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f)),
		mat4().lookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f,  0.0f, -1.0f, 1.0f), vec4(0.0f, -1.0f,  0.0f, 0.0f))
	};

	useProgram();
	updateUniform(m_uni_p, captureProjection);

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentCaptureTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_CAPTURE)->second);
					auto l_environmentPrefilterTexture = textureMap.find(l_graphicData.second.find(textureType::ENVIRONMENT_PREFILTER)->second);
					if (l_environmentCaptureTexture != textureMap.end() && l_environmentPrefilterTexture != textureMap.end())
					{
						// @TODO: it should be update(0)?
						l_environmentCaptureTexture->second->update(2);
						unsigned int maxMipLevels = 5;
						for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
						{
							// reisze framebuffer according to mip-level size.
							unsigned int mipWidth = (int)(128 * std::pow(0.5, mip));
							unsigned int mipHeight = (int)(128 * std::pow(0.5, mip));

							glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
							glViewport(0, 0, mipWidth, mipHeight);

							double roughness = (double)mip / (double)(maxMipLevels - 1);
							updateUniform(m_uni_roughness, roughness);
							for (unsigned int i = 0; i < 6; ++i)
							{
								updateUniform(m_uni_r, captureViews[i]);
								l_environmentPrefilterTexture->second->attachToFramebuffer(0, i, mip);
								glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
								meshMap.find(l_graphicData.first)->second->update();
							}
						}
					}
				}
			}
		}
	}
}

void EnvironmentBRDFLUTPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();
}

void EnvironmentBRDFLUTPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	useProgram();

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_environmentBRDFLUTTexture = textureMap.find(l_graphicData.second.find(textureType::RENDER_BUFFER_SAMPLER)->second);
					if (l_environmentBRDFLUTTexture != textureMap.end())
					{
						l_environmentBRDFLUTTexture->second->attachToFramebuffer(0, 0, 0);
					}
				}
			}
		}
	}
}

void SkyForwardPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_skybox = getUniformLocation("uni_skybox");
	updateUniform(m_uni_skybox, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void SkyForwardPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	useProgram();

	if (cameraComponents.size() > 0)
	{
		mat4 p = cameraComponents[0]->getProjectionMatrix();
		mat4 r = cameraComponents[0]->getInvertRotationMatrix();

		updateUniform(m_uni_p, p);
		updateUniform(m_uni_r, r);
	}

	if (visibleComponents.size() > 0)
	{
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::SKYBOX)
			{
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					auto l_cubeMapTexture = textureMap.find(l_graphicData.second.find(textureType::CUBEMAP)->second);
					if (l_cubeMapTexture != textureMap.end())
					{
						l_cubeMapTexture->second->update(0);
						meshMap.find(l_graphicData.first)->second->update();
					}
				}
			}
		}
	}
	glDepthFunc(GL_LESS);
}

void DebuggerShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	updateUniform(m_uni_normalTexture, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void DebuggerShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	useProgram();

	if (cameraComponents.size() > 0)
	{
		mat4 p = cameraComponents[0]->getProjectionMatrix();
		mat4 r = cameraComponents[0]->getInvertRotationMatrix();
		mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

		updateUniform(m_uni_p, p);
		updateUniform(m_uni_r, r);
		updateUniform(m_uni_t, t);
	}

	if (cameraComponents.size() > 0)
	{
		for (auto& l_cameraComponent : cameraComponents)
		{
			// draw frustum for cameraComponent
			if (l_cameraComponent->m_drawFrustum)
			{
				auto l_cameraLocalMat = mat4();
				l_cameraLocalMat.initializeToIdentityMatrix();
				updateUniform(m_uni_m, l_cameraLocalMat);
				meshMap.find(l_cameraComponent->m_FrustumMeshID)->second->update();
			}
			// draw AABB of frustum for cameraComponent
			if (l_cameraComponent->m_drawAABB)
			{
				auto l_cameraLocalMat = mat4();
				l_cameraLocalMat.initializeToIdentityMatrix();
				updateUniform(m_uni_m, l_cameraLocalMat);
				meshMap.find(l_cameraComponent->m_AABBMeshID)->second->update();
			}
		}
	}

	if (lightComponents.size() > 0)
	{
		// draw AABB for lightComponent
		for (auto& l_lightComponent : lightComponents)
		{
			if (l_lightComponent->m_drawAABB)
			{
				auto l_cameraLocalMat = mat4();
				l_cameraLocalMat.initializeToIdentityMatrix();
				updateUniform(m_uni_m, l_cameraLocalMat);
				//updateUniform(m_uni_m, l_lightComponent->getParentEntity()->caclTransformationMatrix());
				meshMap.find(l_lightComponent->m_AABBMeshID)->second->update();
			}
		}
	}

	if (visibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::STATIC_MESH && l_visibleComponent->m_drawAABB)
			{
				auto l_m = mat4();
				l_m.initializeToIdentityMatrix();
				updateUniform(m_uni_m, l_m);

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
							l_textureData->update(0);
						}
					}
					// draw meshes
					meshMap.find(l_visibleComponent->m_AABBMeshID)->second->update();
				}
			}
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}


void SkyDeferPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_lightPassRT0 = getUniformLocation("uni_lightPassRT0");
	updateUniform(m_uni_lightPassRT0, 0);
	m_uni_skyForwardPassRT0 = getUniformLocation("uni_skyForwardPassRT0");
	updateUniform(m_uni_skyForwardPassRT0, 1);
	m_uni_debuggerPassRT0 = getUniformLocation("uni_debuggerPassRT0");
	updateUniform(m_uni_debuggerPassRT0, 2);
}

void SkyDeferPassPBSShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	useProgram();
}

void BillboardPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_texture = getUniformLocation("uni_texture");
	updateUniform(m_uni_texture, 0);
	m_uni_pos = getUniformLocation("uni_pos");
	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_size = getUniformLocation("uni_size");
	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void BillboardPassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glEnable(GL_DEPTH_TEST);
	useProgram();

	if (cameraComponents.size() > 0)
	{
		mat4 p = cameraComponents[0]->getProjectionMatrix();
		mat4 r = cameraComponents[0]->getInvertRotationMatrix();
		mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

		updateUniform(m_uni_p, p);
		updateUniform(m_uni_r, r);
		updateUniform(m_uni_t, t);
	}

	if (visibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::BILLBOARD)
			{
				updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());
				updateUniform(m_uni_pos, l_visibleComponent->getParentEntity()->getTransform()->getPos().x, l_visibleComponent->getParentEntity()->getTransform()->getPos().y, l_visibleComponent->getParentEntity()->getTransform()->getPos().z);
				updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);
				auto l_distanceToCamera = (cameraComponents[0]->getParentEntity()->getTransform()->getPos() - l_visibleComponent->getParentEntity()->getTransform()->getPos()).length();
				if (l_distanceToCamera > 1)
				{
					updateUniform(m_uni_size, (vec2(1.0, 1.0) * (1.0 / l_distanceToCamera)).x, (vec2(1.0, 1.0) * (1.0 / l_distanceToCamera)).y);
				}
				else
				{
					updateUniform(m_uni_size, vec2(1.0, 1.0).x, vec2(1.0, 1.0).y);
				}

				// draw each graphic data of visibleComponent
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					//active and bind textures
					// is there any texture?
					auto l_textureMap = l_graphicData.second;
					if (&l_textureMap != nullptr)
					{
						// any albedo?
						auto l_diffuseTextureID = l_textureMap.find(textureType::ALBEDO);
						if (l_diffuseTextureID != l_textureMap.end())
						{
							auto& l_textureData = textureMap.find(l_diffuseTextureID->second)->second;
							l_textureData->update(0);
						}
					}
					// draw meshes
					meshMap.find(l_graphicData.first)->second->update();
				}
			}
		}
	}
}

void EmissivePassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void EmissivePassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glEnable(GL_DEPTH_TEST);
	useProgram();

	if (cameraComponents.size() > 0)
	{
		mat4 p = cameraComponents[0]->getProjectionMatrix();
		mat4 r = cameraComponents[0]->getInvertRotationMatrix();
		mat4 t = cameraComponents[0]->getInvertTranslationMatrix();

		updateUniform(m_uni_p, p);
		updateUniform(m_uni_r, r);
		updateUniform(m_uni_t, t);
	}

	if (visibleComponents.size() > 0)
	{
		// draw each visibleComponent
		for (auto& l_visibleComponent : visibleComponents)
		{
			if (l_visibleComponent->m_visiblilityType == visiblilityType::EMISSIVE)
			{
				updateUniform(m_uni_m, l_visibleComponent->getParentEntity()->caclTransformationMatrix());
				updateUniform(m_uni_albedo, l_visibleComponent->m_albedo.x, l_visibleComponent->m_albedo.y, l_visibleComponent->m_albedo.z);

				// draw each graphic data of visibleComponent
				for (auto& l_graphicData : l_visibleComponent->getModelMap())
				{
					// draw meshes
					meshMap.find(l_graphicData.first)->second->update();
				}
			}
		}
	}
}

void FinalPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	useProgram();

	m_uni_skyDeferPassRT0 = getUniformLocation("uni_skyDeferPassRT0");
	updateUniform(m_uni_skyDeferPassRT0, 0);
	m_uni_billboardPassRT0 = getUniformLocation("uni_billboardPassRT0");
	updateUniform(m_uni_billboardPassRT0, 1);
	m_uni_emissivePassRT0 = getUniformLocation("uni_emissivePassRT0");
	updateUniform(m_uni_emissivePassRT0, 2);
	
}

void FinalPassShaderProgram::update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	useProgram();
}

