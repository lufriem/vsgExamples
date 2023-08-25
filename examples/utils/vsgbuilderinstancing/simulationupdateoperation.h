#pragma once

#include <vsg/all.h>

#include "simulationmodel.h"
#include "simulationupdatevisitor.h"

namespace Simulation
{
    // this executes the Simulation::Model once per frame and then uses its 
    // Simulation::UpdateVisitor to to update the scene
    //
    // in principle the model could be run at a different frequency (not coupled 
    // to the frame rate)
    struct UpdateOperation : public vsg::Inherit<vsg::Operation, UpdateOperation>
    {
    public:
        UpdateOperation(vsg::ref_ptr<vsg::Group> scene, Simulation::Model* simulationModel) : 
            scene(scene), model(simulationModel), simUpdateVisitor(simulationModel)
        {
        }

        virtual void run() override
        {
            if (model)
                model->Update();

            scene->accept(simUpdateVisitor);
        }

        // used to traverse the scene once, without running the model (required to set up buffer dataVariance)
        void initializeScene()
        {
            scene->accept(simUpdateVisitor);
        }

    private:
        Simulation::UpdateVisitor simUpdateVisitor;
        vsg::ref_ptr<vsg::Group> scene;
        Simulation::Model* model = nullptr;
    };
}