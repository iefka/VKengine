#include"DE_Device.hpp"
#include"Texture.hpp"
#include "TextureManager.hpp"



namespace de {
    std::shared_ptr<Texture> TextureManager::getTexture(std::string texturePath){

        //trying implace element
        auto [it,success] = cache_.try_emplace(texturePath);

        //creating shared ptr if it value dosent exist then pointer == nullptr
        std::shared_ptr texture = it->second.lock();

        //checking if nullptr
        if (!texture) {
            //if nullptr creating Texture
            texture = std::make_shared<Texture>(texturePath, device_);

            //asign texture to manager element
            it->second = texture;
        }
        
        //return shared ptr
        return texture;
    }
}
