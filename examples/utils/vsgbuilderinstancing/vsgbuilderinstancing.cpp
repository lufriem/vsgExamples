#include <vsg/all.h>

#ifdef vsgXchange_FOUND
#    include <vsgXchange/all.h>
#endif

#include <iostream>

#include "simulationmodel.h"
#include "simulationupdateoperation.h"

#define ENABLE_BILLBOARDING() 0

int main(int argc, char** argv)
{
    auto options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->sharedObjects = vsg::SharedObjects::create();

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "vsgbuilder instancing";

    auto builder = vsg::Builder::create();
    builder->options = options;

    // set up defaults and read command line arguments to override them
    vsg::CommandLine arguments(&argc, argv);
    windowTraits->debugLayer = arguments.read({"--debug", "-d"});
    windowTraits->apiDumpLayer = arguments.read({"--api", "-a"});

    vsg::GeometryInfo geomInfo;
    geomInfo.dx.set(1.0f, 0.0f, 0.0f);
    geomInfo.dy.set(0.0f, 1.0f, 0.0f);
    geomInfo.dz.set(0.0f, 0.0f, 1.0f);

    vsg::StateInfo stateInfo;

    arguments.read("--screen", windowTraits->screenNum);
    arguments.read("--display", windowTraits->display);
    auto numFrames = arguments.value(-1, "-f");
    if (arguments.read({"--fullscreen", "--fs"})) windowTraits->fullscreen = true;
    if (arguments.read({"--window", "-w"}, windowTraits->width, windowTraits->height)) { windowTraits->fullscreen = false; }
    if (arguments.read("--IMMEDIATE")) windowTraits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (arguments.read("--double-buffer")) windowTraits->swapchainPreferences.imageCount = 2;
    if (arguments.read("--triple-buffer")) windowTraits->swapchainPreferences.imageCount = 3; // default
    if (arguments.read("-t"))
    {
        windowTraits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        windowTraits->width = 192, windowTraits->height = 108;
        windowTraits->decoration = false;
    }

    if (arguments.read("--shared")) options->sharedObjects = vsg::SharedObjects::create();

    auto outputFilename = arguments.value<std::string>("", "-o");

    stateInfo.wireframe = arguments.read("--wireframe");
    stateInfo.lighting = !arguments.read("--flat");
    stateInfo.two_sided = arguments.read("--two-sided");

    vsg::vec4 specularColor;
    bool hasSpecularColor = arguments.read("--specular", specularColor);
    vsg::vec4 diffuseColor;
    bool hasDiffuseColor = arguments.read("--diffuse", diffuseColor);
    if (stateInfo.lighting && (hasDiffuseColor || hasSpecularColor))
    {
        builder->shaderSet = vsg::createPhongShaderSet(options);
        if (auto& materialBinding = builder->shaderSet->getUniformBinding("material"))
        {
            auto mat = vsg::PhongMaterialValue::create();
            if (hasSpecularColor)
            {
                vsg::info("specular = ", specularColor);
                mat->value().specular = specularColor;
            }
            if (hasDiffuseColor)
            {
                vsg::info("diffuse= ", diffuseColor);
                mat->value().diffuse = diffuseColor;
            }
            materialBinding.data = mat;
            vsg::info("using custom material ", mat);
        }
    }

    arguments.read("--dx", geomInfo.dx);
    arguments.read("--dy", geomInfo.dy);
    arguments.read("--dz", geomInfo.dz);
#if ENABLE_BILLBOARDING()
    bool billboard = arguments.read("--billboard");
#endif
    auto numVertices = arguments.value<uint32_t>(3000, "-n");
    if (numVertices == 0)
        numVertices = 1;

    vsg::Path textureFile = arguments.value(vsg::Path{}, {"-i", "--image"});
    vsg::Path displacementFile = arguments.value(vsg::Path{}, "--dm");

    if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);

#ifdef vsgXchange_all
    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());
#endif

    auto scene = vsg::Group::create();

    vsg::dvec3 centre = {0.0, 0.0, 0.0};
    double radius = 1.0;

    auto model = Simulation::Model(numVertices, centre, vsg::length(geomInfo.dx));

    radius = vsg::length(geomInfo.dx + geomInfo.dy + geomInfo.dz);

    if (textureFile) stateInfo.image = vsg::read_cast<vsg::Data>(textureFile, options);
    if (displacementFile) stateInfo.displacementMap = vsg::read_cast<vsg::Data>(displacementFile, options);

    if (numVertices == 0) numVertices = 1;
#if ENABLE_BILLBOARDING()
    if (billboard)
    {
        stateInfo.billboard = true;

        float w = std::pow(float(numVertices), 0.33f) * 2.0f * vsg::length(geomInfo.dx);
        float scaleDistance = w*3.0f;
        auto positions = vsg::vec4Array::create(numVertices);
        geomInfo.positions = positions;
                
        for (auto& v : *(positions))
        {
            v.set(w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                w * (float(std::rand()) / float(RAND_MAX) - 0.5f), scaleDistance);
        }

        radius += (0.5 * sqrt(3.0) * w);
    }
    else
#endif
    {
        stateInfo.instance_positions_vec3 = true;

        auto [pos, w] = model.GetSpherePositions();
        geomInfo.positions = pos;

        radius += (0.5 * sqrt(3.0) * w);
    }

    auto colors = vsg::vec4Array::create(numVertices);
    geomInfo.colors = colors;
    for (auto& c : *(colors))
    {
        c.set(float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), 1.0f);
    }

    vsg::dbox bound;
    scene->addChild(builder->createSphere(geomInfo, stateInfo));
    bound.add(geomInfo.position);
    geomInfo.position += geomInfo.dx * 1.5f;

    // update the centre and radius to account for all the shapes added so we can position the camera to see them all.
    centre = (bound.min + bound.max) * 0.5;
    radius += vsg::length(bound.max - bound.min) * 0.5;

    // write out scene if required
    if (!outputFilename.empty())
    {
        vsg::write(scene, outputFilename, options);
        return 0;
    }

    // create the viewer and assign window(s) to it
    auto viewer = vsg::Viewer::create();

    auto window = vsg::Window::create(windowTraits);
    if (!window)
    {
        std::cout << "Could not create window." << std::endl;
        return 1;
    }

    viewer->addWindow(window);

    // set up the camera
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));

    double nearFarRatio = 0.001;
    auto perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);

    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // add close handler to respond to the close window button and pressing escape
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));

    viewer->addEventHandler(vsg::Trackball::create(camera));

    // assign an update operation, which runs our operation model and updates the scene once per frame
    auto updateOperation = Simulation::UpdateOperation::create(scene, &model);
    viewer->addUpdateOperation(updateOperation, vsg::UpdateOperations::ALL_FRAMES);

    // traverse the scene once to set all instance buffer data instances to have 
    // DYNAMIC data variance.. this needs to be done BEFORE the 'Viewer::compile' call
    updateOperation->initializeScene();

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->compile();

    auto startTime = vsg::clock::now();
    double numFramesCompleted = 0.0;

    // rendering main loop
    while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
    {
        // pass any events into EventHandlers assigned to the Viewer
        viewer->handleEvents();

        viewer->update();

        viewer->recordAndSubmit();

        viewer->present();

        numFramesCompleted += 1.0;
    }

    auto duration = std::chrono::duration<double, std::chrono::seconds::period>(vsg::clock::now() - startTime).count();
    if (numFramesCompleted > 0.0)
    {
        std::cout << "Average frame rate = " << (numFramesCompleted / duration) << std::endl;
    }

    return 0;
}
