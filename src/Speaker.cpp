// Speaker.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "Tile.hpp"
#include "Speaker.hpp"
#include "Sound.hpp"
#include "Camera.hpp"
#include "BBox.hpp"
#include "Client.hpp"
#include "Mixer.hpp"

const char* Speaker::meshStr = "assets/editor/speaker/speaker.FBX";
const char* Speaker::materialStr = "assets/editor/speaker/material.json";

Speaker::Speaker(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	for (int i = 0; i < maxSources; ++i) {
		sources[i] = 0;
	}

	name = typeStr[COMPONENT_SPEAKER];

	// add a bbox for editor usage
	if (mainEngine->isEditorRunning()) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_SPHERE);
		bbox->setLocalScale(Vector(16.f));
		bbox->setEditorOnly(true);
		bbox->update();
	}

	// exposed attributes
	attributes.push(new AttributeFile("Default Sound", "wav,ogg,mp3,FLAC,mod", defaultSound));
	attributes.push(new AttributeBool("Loop", defaultLoop));
	attributes.push(new AttributeFloat("Range", defaultRange));
}

Speaker::~Speaker() {
	stopAllSounds();
}

int Speaker::playSound(const char* _name, const bool loop, float range) {
	if (!_name || strlen(_name) == 0) {
		return -1;
	}
	if (!entity->getWorld()) {
		return -1;
	}
	if (!entity->getWorld()->isClientObj()) {
		return -1;
	}

	Sound* sound = mainEngine->getSoundResource().dataForString(StringBuf<64>("sounds/%s", 1, _name).get());
	if (sound) {
		int index = maxSources;
		for (int i = 0; i < maxSources; ++i) {
			if (sources[i] == 0) {
				index = i;
				break;
			}
		}
		if (index == maxSources) {
			mainEngine->fmsg(Engine::MSG_WARN, "'%s' cannot play sound '%s', no more available sources.", name.get(), _name);
			return -1;
		}
		alGenSources(1, &sources[index]);
		alSourcei(sources[index], AL_BUFFER, sound->getBuffer());
		alSourcef(sources[index], AL_PITCH, 1.0f);
		alSourcef(sources[index], AL_GAIN, 0.0f);
		if (loop) {
			alSourcei(sources[index], AL_LOOPING, AL_TRUE);
		} else {
			alSourcei(sources[index], AL_LOOPING, AL_FALSE);
		}

		// position and orient the sound in 3D space
		auto& m = gMat;
		ALfloat orientation[6] = { m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2] };

		float f = 2.f / Tile::size;
		alSource3f(sources[index], AL_POSITION, gPos.x*f, -gPos.z*f, gPos.y*f);
		alSource3f(sources[index], AL_VELOCITY, entity->getVel().x*f, -entity->getVel().z*f, entity->getVel().y*f);
		alSourcefv(sources[index], AL_DIRECTION, orientation);
		alSourcef(sources[index], AL_REFERENCE_DISTANCE, range / Tile::size);
		//alSourcef(sources[index], AL_MAX_DISTANCE, range / Tile::size);

		// play the sound
		alSourcePlay(sources[index]);
		return index;
	} else {
		mainEngine->fmsg(Engine::MSG_WARN, "'%s' could not play sound '%s', sound not cached.", name.get(), _name);
		return -1;
	}
}

bool Speaker::isPlayingAnything() const {
	for (int i = 0; i < maxSources; ++i) {
		if (isPlaying(i)) {
			return true;
		}
	}
	return false;
}

bool Speaker::isPlaying(int index) const {
	if (!sources[index]) {
		return false;
	}
	ALint state = AL_PLAYING;
	alGetSourcei(sources[index], AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING) {
		return true;
	} else {
		return false;
	}
}

bool Speaker::stopSound(const int index) {
	if (index < 0 || index >= maxSources)
		return false;
	if (!sources[index])
		return false;
	if (!isPlaying(index))
		return false;
	alSourceStop(sources[index]);
	alDeleteSources(1, &sources[index]);
	sources[index] = 0;
	return true;
}

bool Speaker::stopAllSounds() {
	bool result = false;
	for (int i = 0; i < maxSources; ++i) {
		if (stopSound(i)) {
			result = true;
		}
	}
	return result;
}

void Speaker::draw(Camera& camera, const ArrayList<Light*>& lights) {
	// only render in the editor!
	if (!mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho()) {
		return;
	}

	// do not render for these fx passes
	if (camera.getDrawMode() >= Camera::DRAW_GLOW) {
		return;
	}

	glm::mat4 matrix = glm::translate(glm::mat4(1.f), glm::vec3(0, -16.f, -4.f));
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr);
	Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
	if (mesh && material) {
		ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, gMat * matrix);
		if (shader) {
			mesh->draw(camera, this, shader);
		}
	}
}

static Cvar cvar_speakerCull("speaker.cull", "accuracy for speakers' occlusion culling", "7");

void Speaker::process() {
	Component::process();

	// update sound sources
	for (int i = 0; i < maxSources; ++i) {
		if (sources[i]) {
			ALint state = AL_STOPPED;
			alGetSourcei(sources[i], AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING) {
				auto& m = gMat;
				ALfloat orientation[6] = { m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2] };

				float f = 2.f / Tile::size;
				alSource3f(sources[i], AL_POSITION, gPos.x*f, -gPos.z*f, gPos.y*f);
				alSource3f(sources[i], AL_VELOCITY, entity->getVel().x*f, -entity->getVel().z*f, entity->getVel().y*f);
				alSourcefv(sources[i], AL_DIRECTION, orientation);

				bool inWorld = false;
				Camera* camera = nullptr;
				Mixer* mixer = nullptr;
				if (entity->getWorld() != nullptr) {
					if (!entity->getWorld()->isShowTools() || !mainEngine->isEditorRunning()) {
						Client* client = mainEngine->getLocalClient();
						if (client) {
							mixer = client->getMixer();
							if (mixer) {
								camera = mixer->getListener();
								if (camera) {
									const Entity* listener = camera->getEntity();
									if (listener) {
										if (listener->getWorld() == entity->getWorld()) {
											inWorld = true;
										}
									}
								}
							}
						}
					}
				}

				if (inWorld) {
					if (camera->seesEntity(*entity, camera->getClipFar(), cvar_speakerCull.toInt())) {
						alSourcei(sources[i], AL_DIRECT_FILTER, 0);
						alSourcef(sources[i], AL_GAIN, 1.f);
					} else {
						alSourcei(sources[i], AL_DIRECT_FILTER, mixer->getLowpassFilter());
						alSourcef(sources[i], AL_GAIN, 1.0f);
					}
				} else {
					alSourcef(sources[i], AL_GAIN, 0.f);
				}
			} else {
				alDeleteSources(1, &sources[i]);
				sources[i] = 0;
			}
		}
	}
}

void Speaker::load(FILE* fp) {
	Component::load(fp);

	Uint32 len = 0;
	char* defaultSoundStr = nullptr;
	Engine::freadl(&len, sizeof(Uint32), 1, fp, nullptr, "Speaker::load()");
	if (len > 0 && len < 128) {
		defaultSoundStr = (char *)calloc(len + 1, sizeof(char));
		Engine::freadl(defaultSoundStr, sizeof(char), len, fp, nullptr, "Speaker::load()");
	} else if (len >= 128) {
		assert(0);
	}
	defaultSound = defaultSoundStr;
	free(defaultSoundStr);

	Engine::freadl(&defaultLoop, sizeof(bool), 1, fp, nullptr, "Speaker::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Speaker::load()");

	// new data 2017/12/04
	if (reserved == 1) {
		Engine::freadl(&defaultRange, sizeof(float), 1, fp, nullptr, "Speaker::load()");

		reserved = 0;
		Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Speaker::load()");
	}

	if (!defaultSound.empty()) {
		playSound(defaultSound, defaultLoop, defaultRange);
	}

	loadSubComponents(fp);
}

void Speaker::serialize(FileInterface * file) {
	Component::serialize(file);

	Uint32 version = 0;
	file->property("Speaker::version", version);
	file->property("defaultSound", defaultSound);
	file->property("defaultLoop", defaultLoop);
	file->property("defaultRange", defaultRange);

	if (file->isReading()) {
		if (!defaultSound.empty()) {
			playSound(defaultSound, defaultLoop, defaultRange);
		}
	}
}