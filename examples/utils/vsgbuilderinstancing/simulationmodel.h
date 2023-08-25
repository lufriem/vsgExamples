#pragma once

#include <vsg/all.h>

namespace Simulation
{
    // Simplistic 'simulation' which generates some data that can be rendered
    // It:
    // - changes the color of one sphere per frame (until all instances have been changed, then a new color gets computed and the cycle continues anew)
    // - (re)calculates the position of a single sphere (this one has a special color). Once this sphere leaves its designated moving volume
    //   it gets 'bounced back' and continues its travel
    //
    // the simulation keeps track of all spheres' positions (even though only one changes) but it doesn't care about the individual sphere's 
    // color
    class Model
    {
    public:
        Model(uint32_t instanceCount, const vsg::dvec3& centre, float length) :
            instanceCount(instanceCount), centre(centre), movingSphereColor(1,1,1,1), movingSphereIndex(instanceCount/2)
        {
            sphereColor = vsg::vec4(0,0,1,1); 

            movingSphereTravelDirection.set(float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX));

            CreatePositions(length);
        }

        void Update()
        {
            UpdateNonMovingSphereColor();
            UpdateMovingSpherePosition();
        }

        vsg::vec4 GetColorForSphere(uint32_t index) const
        {
            return (index != GetMovingSphereIndex()) ? sphereColor : movingSphereColor;
        }

        vsg::vec3 GetMovingSpherePosition() const
        {
            return (*spherePositions)[GetMovingSphereIndex()];
        }

        uint32_t GetMovingSphereIndex() const
        {
            return movingSphereIndex;
        }

        // the index for the instance whose color gets set this frame
        uint32_t GetCurrentActiveIndex() const
        {
            return activeSphereIndex;
        }

        std::pair<vsg::ref_ptr<vsg::vec3Array>, float> GetSpherePositions() const
        { 
            return { spherePositions, sphereVolumeSizeLength };
        }

    private:
        void UpdateNonMovingSphereColor()
        {
            activeSphereIndex++;
            if (activeSphereIndex > instanceCount)
            {
               RecalculateSphereColor();
            }
        }

        void UpdateMovingSpherePosition()
        {
            auto &position = (*spherePositions)[GetMovingSphereIndex()];
            position += movingSphereTravelDirection * 0.025f;

            // if our sphere is outside its travel volume, bounce it back
            if (vsg::length(position - centre) > (sphereVolumeSizeLength/2))
            {
                vsg::vec3 newDir(0 - float(std::rand()) / float(RAND_MAX),
                                 0 - float(std::rand()) / float(RAND_MAX),
                                 0 - float(std::rand()) / float(RAND_MAX));

                movingSphereTravelDirection *= newDir;
                movingSphereTravelDirection = vsg::normalize(movingSphereTravelDirection);

                position += movingSphereTravelDirection * 0.05f;    // move it back into our volume
            }
        }

        void RecalculateSphereColor()
        {
            sphereColor.set(float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), 1.0f);
            activeSphereIndex = 0;
        }

        void CreatePositions(float length)
        {
            auto w = std::pow(float(instanceCount), 0.33f) * 2.0f * length;
            spherePositions = vsg::vec3Array::create(instanceCount);
        
            for (auto& v : *(spherePositions))
            {
                v.set(w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f));
            }

            (*spherePositions)[GetMovingSphereIndex()] = centre;
            sphereVolumeSizeLength = w;
        }
    
        const uint32_t instanceCount;
        uint32_t activeSphereIndex = 0; // changes once per frame
        vsg::vec4 sphereColor;          // for all spheres
    
        const vsg::vec3 centre;
        float sphereVolumeSizeLength = 0;
        vsg::ref_ptr<vsg::vec3Array> spherePositions;   // for all spheres (including the moving sphere)

        const vsg::vec4 movingSphereColor;
        vsg::vec3 movingSphereTravelDirection;  // normalized vector

        const int movingSphereIndex;
    };
}