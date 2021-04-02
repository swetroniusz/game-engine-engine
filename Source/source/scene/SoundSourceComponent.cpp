#include <scene/SoundSourceComponent.h>
#include <rendering/Material.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

SoundSourceComponent::SoundSourceComponent(GameScene& scene, std::string name, SoundBuffer* buffer, Transform transform):
	Component(scene, name, transform), ALIndex(0), BufferPtr(buffer)
{
	if (buffer)
		GenAL(buffer);
}

void SoundSourceComponent::GenAL(SoundBuffer* buffer)
{
	if (buffer)
		BufferPtr = buffer;
	if (!BufferPtr)
	{
		std::cerr << "ERROR! No AL buffer is assigned to a sound source; Can't generate AL source.\n";
		return;
	}

	if (ALIndex != 0)
		alDeleteSources(1, &ALIndex);

	alGenSources(1, &ALIndex);
	alSourcefv(ALIndex, AL_POSITION, glm::value_ptr(glm::vec3(0.0f)));
	alSourcefv(ALIndex, AL_VELOCITY, glm::value_ptr(glm::vec3(0.0f)));
	alSourcei(ALIndex, AL_BUFFER, BufferPtr->ALIndex);
}

void SoundSourceComponent::SetLoop(bool loop)
{
	alSourcei(ALIndex, AL_LOOPING, (loop) ? (AL_TRUE) : (AL_FALSE));
}

bool SoundSourceComponent::IsPlaying()
{
	ALint state;
	alGetSourcei(ALIndex, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING)
		return true;

	return false;
}

void SoundSourceComponent::Play()
{
	if (!BufferPtr)
	{
		std::cerr << "ERROR! No AL buffer is assigned to a sound source; Can't play the sound.\n";
		return;
	}

	if (IsPlaying())
		return;

	std::cout << "Gram dzwiek " << Name << '\n';
	GameHandle->GetAudioEngineHandle()->CheckError();

	alSourcePlay(ALIndex);
}

void SoundSourceComponent::Pause()
{
	alSourcePause(ALIndex);
}

void SoundSourceComponent::Stop()
{
	alSourceStop(ALIndex);
}

void SoundSourceComponent::Update(float deltaTime)
{
	alSourcefv(ALIndex, AL_POSITION, glm::value_ptr(ComponentTransform.GetWorldTransform().PositionRef));
}

MaterialInstance SoundSourceComponent::GetDebugMatInst(EditorIconState state)
{
	 LoadDebugRenderMaterial("GEE_Mat_Default_Debug_SoundSourceComponent", "EditorAssets/soundsourcecomponent_debug.png");
	 return Component::GetDebugMatInst(state);
}

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <assetload/FileLoader.h>

void SoundSourceComponent::GetEditorDescription(UIActor& editorParent, GameScene& editorScene)
{
	Component::GetEditorDescription(editorParent, editorScene);

	AddFieldToCanvas("Path", editorParent).GetTemplates().PathInput([this](const std::string& path) {GameHandle->GetAudioEngineHandle()->CheckError(); GenAL(GameHandle->GetAudioEngineHandle()->LoadBufferFromFile(path)); GameHandle->GetAudioEngineHandle()->CheckError(); }, [this]()->std::string { if (BufferPtr) return BufferPtr->Path; return std::string(); });
}

void SoundSourceComponent::Dispose()
{
	if (IsPlaying())
		alSourceStop(ALIndex);

	alDeleteSources(1, &ALIndex);
}