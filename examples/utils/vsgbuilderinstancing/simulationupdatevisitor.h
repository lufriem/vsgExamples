#pragma once

#include <vsg/all.h>

#include "simulationmodel.h"

namespace Simulation
{
    struct UpdateVisitor : public vsg::Visitor
    {
        UpdateVisitor(Simulation::Model* simulationModel) : 
            model(simulationModel)
        {    
        }

        void apply(vsg::Object& object) override
        {
            object.traverse(*this);
        }

        void apply(vsg::VertexIndexDraw& vid) override
        {
            if (model)
            {
                /*
                To update the positions after vsg::Builder has set things up will require you to 
                traverse the subgraph to find the VertexIndexDraw node then travers its arrays to 
                find the vsg::BufferInfo object that has data equal to the GeometryInfo.positions vec3array.  
            
                This BufferInfo is the one you'll need to use when updating the array
                */
                if (vid.instanceCount > 0)  // note: this assumes that there's only a single instanced node..
                {
                    // now look up our instance data, in the node's BufferInfoList array
                    // as we see from the Builder::createSphere function
                    // index 3 are the colors
                    // index 4 are the positions
                    
                    // first set the currently active sphere's color
                    {
                        auto colorData = reinterpret_cast<vsg::Array<vsg::vec4>*>(vid.arrays[3]->data.get());

                        if (firstLoop)
                            colorData->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;

                        // note: for the Simulation::movingSphereIndex index we only need to set this once (in the first loop) in principle..
                        auto currentUpdateIndex = model->GetCurrentActiveIndex();
                        colorData->set(currentUpdateIndex, model->GetColorForSphere(currentUpdateIndex));

                        colorData->dirty();
                    }

                    // then move our (one) moving sphere
                    {
                        auto positionData = reinterpret_cast<vsg::vec3Array*>(vid.arrays[4]->data.get());

                        if (firstLoop)
                            positionData->properties.dataVariance = vsg::DataVariance::DYNAMIC_DATA;

                        positionData->set(model->GetMovingSphereIndex(), model->GetMovingSpherePosition());

                        positionData->dirty();
                    }

                    firstLoop = false;
                }
            }

            vid.traverse(*this);
        }

    private:
        Simulation::Model* model = nullptr;
        bool firstLoop = true; // the pass before Viewer::compile was called..
    };
}