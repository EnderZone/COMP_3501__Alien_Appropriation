#include "player_node.h"
#include "scene_graph.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace game {
	PlayerNode::PlayerNode(const std::string name, const Resource *geometry, const Resource *material, const Resource *texture) : SceneNode(name, geometry, material, texture),
		forward_factor(40.0f),
		x_tilt_percentage(0.0f),
		y_tilt_percentage(0.0f),
		tractor_beam_on(false),
		shielding_on(false),
		cowsCollected(0),
		hayCollected(0)
	{
		// Set This as the parentNode of the camera while taking its own parent as his
		//camera->addChildNode(this);
		radius = 2;
	}

	PlayerNode::~PlayerNode() {}

	glm::vec3 PlayerNode::getPosition(void)
	{
		return mPosition + (dynamic_cast<Camera*>(getParentNode()))->getPosition();
	}

	void PlayerNode::rotateLeft() {
		x_tilt_percentage += glm::pi<float>() / 20.0f;
		x_tilt_percentage = glm::min(glm::pi<float>() / 2.0f, x_tilt_percentage);
	}

	void PlayerNode::rotateRight() {
		x_tilt_percentage -= glm::pi<float>() / 20.0f;
		x_tilt_percentage = glm::max(-glm::pi<float>() / 2.0f, x_tilt_percentage);
	}

	void PlayerNode::rotateForward() {
		y_tilt_percentage += glm::pi<float>() / 20.0f;
		y_tilt_percentage = glm::min(glm::pi<float>() / 2.0f, y_tilt_percentage);
	}

	void PlayerNode::rotateBackward() {
		y_tilt_percentage -= glm::pi<float>() / 20.0f;
		y_tilt_percentage = glm::max(-glm::pi<float>() / 2.0f, y_tilt_percentage);
	}

	void PlayerNode::rotateByCamera() {
		float velocity_limit = glm::pi<float>() / 2.0f;
		
		glm::vec3 current_velocity = ((Camera*)this->getParentNode())->getVelocityRaw();

		x_tilt_percentage = -current_velocity.x * velocity_limit;
		y_tilt_percentage = current_velocity.z * velocity_limit;
	
		x_tilt_percentage = glm::max(-1.0f, x_tilt_percentage);
		x_tilt_percentage = glm::min( 1.0f, x_tilt_percentage);
		
		y_tilt_percentage = glm::max(-1.0f, y_tilt_percentage);
		y_tilt_percentage = glm::min( 1.0f, y_tilt_percentage);
		
		// std::cout << "PERCENTAGES ::: " << x_tilt_percentage << " " << y_tilt_percentage << std::endl;
	}

	void PlayerNode::draw(SceneNode* camera, glm::mat4 parentTransf) {
		// Select proper material (shader program)
		glUseProgram(mMaterial);

		// Set geometry to draw
		glBindBuffer(GL_ARRAY_BUFFER, mArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementArrayBuffer);

		// Set globals for camera
		camera->SetupShader(mMaterial);

		// Set world matrix and other shader input variables
		SetupShader(mMaterial, parentTransf);

		// draw geometry
		if (mMode == GL_POINTS) {
			glDrawArrays(mMode, 0, mSize);
		}
		else {
			glDrawElements(mMode, mSize, GL_UNSIGNED_INT, 0);
		}

		for (BaseNode* bn : getChildNodes())
		{
			dynamic_cast<SceneNode*>(bn)->draw(camera, parentTransf);
		}

		for (BaseNode *bn : weapons) {
			std::string node_name = bn->getName();
			if (tractor_beam_on && node_name.compare("TRACTORBEAM") == 0) {
				dynamic_cast<SceneNode*>(bn)->draw(camera, parentTransf);
			}
			if (shielding_on && node_name.compare("SHIELD") == 0) {
				dynamic_cast<SceneNode*>(bn)->draw(camera, parentTransf);
			}
		}
	}

	void PlayerNode::update(double deltaTime) {
		SceneNode::update(deltaTime);
		rotateByCamera();
		setPlayerPosition();
		checkWeapons();

		*energy += 5.0f;
		if (*energy < 0.0f) {
			*energy = 0.0f;
		}
		if (*energy > 100.0f) {
			*energy = 100.0f;
		}

		for (BaseNode* bn : getChildNodes())
		{
			bn->update(deltaTime);
		}
		
		for (BaseNode* bn : weapons)
		{
			bn->update(deltaTime);
		}
	}


void PlayerNode::checkWeapons() {


	if (*energy <= 10) {
		toggleShields(false);
		toggleTractorBeam(false);
		return;
	}
	if (tractor_beam_on)
		*energy -= 10.0f;
	if (shielding_on)
		*energy -= 6.0f;

}



void PlayerNode::takeDamage(DamageType damage) {
	*hull_strength -= damage;
}

void PlayerNode::dropBomb()
{
	if (hayCollected > 0) {
		hayCollected--;
		bombCounter++;
		for (BaseNode* bn : getChildNodes())
		{
			if (bn->hasTag("orbitingHay")) {
				bn->addTag("delete");
				break;
			}
		}
		 EntityNode* bomb = SceneGraph::CreateInstance<EntityNode>("hayBomb" + std::to_string(bombCounter), "hayMesh", "litTextureMaterial", "hayTexture");
		 bomb->addTag("bomb");
		 bomb->setPosition(getPosition());
		 bomb->setIsGrounded(false);
	}
}


	void PlayerNode::addCollected(std::string type)
	{
		SceneNode* collected = nullptr;
		std::cout << "Collected " << type << std::endl;
		if (type.compare("hay") == 0) {
			hayCollected++;
			collected = SceneGraph::CreateInstance<SceneNode>("orbiting_hay" + std::to_string(hayCollected), "hayMesh", "litTextureMaterial", "hayTexture", this);
			collected->addTag("orbitingHay");
		}
		else {
			cowsCollected++;
			collected = SceneGraph::CreateInstance<SceneNode>("orbiting_cow" + std::to_string(cowsCollected), "cowMesh", "litTextureMaterial", "cowTexture", this);
		}
		collected->setPosition(glm::vec3(0.0f));
		collected->translate(glm::vec3(2.0f * cos(getChildNodes().size()), 1.0f, 2.0f * sin(getChildNodes().size())));
		collected->scale(glm::vec3(0.25f));
	}



	void PlayerNode::setPlayerPosition() {
		mPosition = -forward_factor * glm::vec3(0.0f, 0.0f, 1.0f);
	}

	float PlayerNode::getDistanceFromCamera() {
		return forward_factor;
	}

	void PlayerNode::SetupShader(GLuint program, glm::mat4& parentTransf /*= glm::mat4(1.0)*/) {

		// Set attributes for shaders
		GLint vertex_att = glGetAttribLocation(program, "vertex");
		glVertexAttribPointer(vertex_att, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(vertex_att);

		GLint normal_att = glGetAttribLocation(program, "normal");
		glVertexAttribPointer(normal_att, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(normal_att);

		GLint color_att = glGetAttribLocation(program, "color");
		glVertexAttribPointer(color_att, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void *)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(color_att);

		GLint tex_att = glGetAttribLocation(program, "uv");
		glVertexAttribPointer(tex_att, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void *)(9 * sizeof(GLfloat)));
		glEnableVertexAttribArray(tex_att);

		// Adding the tilts when moving
		float angle_x = (glm::pi<float>() / 16) * glm::sin(x_tilt_percentage);
		float angle_y = (glm::pi<float>() / 16) * glm::sin(y_tilt_percentage);

		// std::cout << "PERCENTAGES ::: " << x_tilt_percentage << " " << y_tilt_percentage << std::endl;
		// std::cout << "ANGLES ::: " << angle_x << " " << angle_y << std::endl;
		glm::quat current_rotation;
		current_rotation = glm::quat_cast(glm::rotate(glm::mat4(), angle_x, glm::vec3(0.0, 0.0, 1.0)));
		current_rotation = glm::normalize(current_rotation);
		current_rotation *= glm::quat_cast(glm::rotate(glm::mat4(), angle_y, glm::vec3(1.0, 0.0, 0.0)));
		current_rotation = glm::normalize(current_rotation);
		mOrientation *= glm::angleAxis(glm::pi<float>() / 180, glm::vec3(0.0f, 1.0f, 0.0f));


		// Aply transformations *ISROT*
		glm::mat4 scaling = glm::scale(glm::mat4(1.0), mScale);
		glm::mat4 rotation = glm::mat4_cast(current_rotation);
		glm::mat4 translation = glm::translate(glm::mat4(1.0), mPosition);
		glm::mat4 temp_transf = parentTransf * translation * rotation;
		parentTransf *= translation * glm::mat4_cast(glm::normalize(mOrientation));
		// Scaling is done only on local object
		glm::mat4 transf = glm::scale(temp_transf, mScale);

		GLint world_mat = glGetUniformLocation(program, "world_mat");
		glUniformMatrix4fv(world_mat, 1, GL_FALSE, glm::value_ptr(transf));

		// Texture
		if (mTexture) {
			GLint tex = glGetUniformLocation(program, "texture_map");
			glUniform1i(tex, 0); // Assign the first texture to the map
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mTexture); // First texture we bind
			// Define texture interpolation
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		// Texture
		if (mEnvmap) {
			GLint useEnv = glGetUniformLocation(program, "useEnvMap");
			glUniform1i(useEnv, true);
			GLint tex = glGetUniformLocation(program, "env_map");
			glUniform1i(tex, 1); // Assign the first texture to the map
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvmap); // First texture we bind
			// Define texture interpolation
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		// Timer
		GLint timer_var = glGetUniformLocation(program, "timer");
		double current_time = glfwGetTime();
		glUniform1f(timer_var, (float)current_time);
	}

}