#include "SkyDomePlugin.h"
#include <data_broker/DataBrokerInterface.h>
#include <data_broker/DataPackage.h>
#include <mars_interfaces/graphics/GraphicsManagerInterface.h>
#include <mars_interfaces/sim/SimulatorInterface.h>
#include <mars_utils/misc.h>
#include <osg/TextureCubeMap>
#include <osg/PositionAttitudeTransform>
#include <osg/Group>
#include <osgUtil/CullVisitor>
#include <osgDB/ReadFile>

namespace mars
{
    namespace plugin
    {
        namespace SkyDomePlugin
        {

            using namespace mars::utils;
            using namespace mars::interfaces;

            bool SkyTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                         osg::NodeVisitor* nv) const
            {
                osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
                if(cv)
                {
                    osg::Vec3 eyePointLocal = cv->getEyeLocal()*s1;
                    matrix.preMultTranslate(eyePointLocal);
                }
                return true;
            }

            bool SkyTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                         osg::NodeVisitor* nv) const
            {
                osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
                if(cv)
                {
                    osg::Vec3 eyePointLocal = cv->getEyeLocal()*s2;
                    matrix.postMultTranslate(-eyePointLocal);
                }
                return true;
            }

            SkyDomePlugin::SkyDomePlugin(lib_manager::LibManager *theManager)
                : MarsPluginTemplateGUI(theManager, "SkyDomePlugin"), materialManager(0)
            {
            }

            void SkyDomePlugin::init()
            {
                if(!control->graphics)
                {
                    LOG_ERROR("SkyDomePlugin: no control->graphics!");
                    return;
                }
                posTransform = new SkyTransform();
                posTransform->setCullingActive(false);
                updateProp = true;
                scene = static_cast<osg::Group*>(control->graphics->getScene());

                resPath = ".";
                if(control->cfg)
                {
                    cfgProp = control->cfg->getOrCreateProperty("Preferences",
                                                                "resources_path",
                                                                "");
                    if(cfgProp.sValue == "")
                    {
                        cfgProp.sValue = MARS_DEFAULT_RESOURCES_PATH;
                    }
                    resPath = cfgProp.sValue;
                    cfgPropPath = control->cfg->getOrCreateProperty("Scene",
                                                                    "skydome_path",
                                                                    "cubemap",
                                                                    this);
                    folder = cfgPropPath.sValue;
                    cfgEnableSD = control->cfg->getOrCreateProperty("Scene",
                                                                    "skydome_enabled",
                                                                    false, this);
                    cfgProp = control->cfg->getOrCreateProperty("Scene",
                                                                "mesh",
                                                                "");
                    meshPath = cfgProp.sValue;
                    cfgProp = control->cfg->getOrCreateProperty("Scene",
                                                                "mesh_material",
                                                                "");
                    meshMaterialName = cfgProp.sValue;
                    cfgProp = control->cfg->getOrCreateProperty("Scene",
                                                                "mesh_scale",
                                                                1.0);
                    meshScale = cfgProp.dValue;
                } else
                {
                    cfgEnableSD.bValue = false;
                }

                if(cfgEnableSD.bValue)
                {
                    scene->addChild(posTransform);
                }

                if(pathExists(resPath+"/mars_graphics/resources/Textures/"+folder))
                {
                    folder = resPath+"/mars_graphics/resources/Textures/"+folder;
                }
                LOG_INFO("SkyDome: init with folder: %s", folder.c_str());
                if(meshPath != "")
                {
                    if(!pathExists(meshPath))
                    {
                        LOG_ERROR("SkyDome: Mesh path not found: %s", meshPath.c_str());
                        meshPath = "";
                    }
                }

                // todo: handle wrong path
                osg::ref_ptr<osg::TextureCubeMap> _cubemap = loadCubeMapTextures();

                _skyDome = new SkyDome( 1.9f, 24, 24, _cubemap.get() );
                _skyDome->setCullingActive(false);
                posTransform->addChild(_skyDome.get());

                osg::StateSet *states = _skyDome->getOrCreateStateSet();
                states->setMode(GL_LIGHTING,
                                osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
                //states->setMode(GL_BLEND,osg::StateAttribute::OFF);
                states->setMode(GL_FOG, osg::StateAttribute::OFF);
                states->setRenderBinDetails( -3, "RenderBin");
                states->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
                //states->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                //states->setMode(GL_BLEND,osg::StateAttribute::ON);
                materialManager = libManager->getLibraryAs<osg_material_manager::OsgMaterialManager>("osg_material_manager", true);

                // setup mesh
                if(meshPath.size() > 0)
                {
                    meshPos = new SkyTransform();
                    meshPos->s1 = meshScale;
                    meshPos->s2 = 1/meshScale;
                    osg::ref_ptr<osg::Node> mesh = osgDB::readNodeFile(meshPath);
                    mesh->setCullingActive(false);
                    osg::StateSet *states = mesh->getOrCreateStateSet();
                    states->setMode(GL_FOG, osg::StateAttribute::OFF);
                    states->setRenderBinDetails( -2, "RenderBin");
                    states->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
                    if(meshMaterialName != "")
                    {
                        materialGroup = materialManager->getNewMaterialGroup(meshMaterialName);
                    }
                    meshPos->addChild(mesh.get());
                    if(cfgEnableSD.bValue)
                    {
                        if(materialGroup.valid())
                        {
                            materialGroup->addChild(meshPos);
                        } else
                        {
                            scene->addChild(meshPos);
                        }
                    }
                }

                control->sim->switchPluginUpdateMode(0, this);
                gui->addGenericMenuAction("../View/", 0, NULL, 0, "", 0, -1); // separator
                gui->addGenericMenuAction("../View/Sky dome", 1, this, 0, "", 0,
                                          1 + cfgEnableSD.bValue);
            }

            void SkyDomePlugin::reset()
            {
            }

            SkyDomePlugin::~SkyDomePlugin()
            {
                if(materialManager) libManager->releaseLibrary("osg_material_manager");
            }


            osg::ref_ptr<osg::TextureCubeMap> SkyDomePlugin::loadCubeMapTextures()
            {
                enum {POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z};
                std::string filenames[6];
                filenames[POS_X] = folder + "/east.png";
                filenames[NEG_X] = folder + "/west.png";
                filenames[POS_Z] = folder + "/north.png";
                filenames[NEG_Z] = folder + "/south.png";
                filenames[POS_Y] = folder + "/down.png";
                filenames[NEG_Y] = folder + "/up.png";

                osg::ref_ptr<osg::TextureCubeMap> cubeMap = new osg::TextureCubeMap;
                cubeMap->setInternalFormat(GL_RGBA);

                cubeMap->setFilter( osg::Texture::MIN_FILTER,
                                    osg::Texture::LINEAR_MIPMAP_LINEAR);
                cubeMap->setFilter( osg::Texture::MAG_FILTER,
                                    osg::Texture::LINEAR);
                cubeMap->setWrap  ( osg::Texture::WRAP_S,
                                    osg::Texture::CLAMP_TO_EDGE);
                cubeMap->setWrap  ( osg::Texture::WRAP_T,
                                    osg::Texture::CLAMP_TO_EDGE);

                cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_X,
                                  osgDB::readImageFile( filenames[NEG_X] ) );
                cubeMap->setImage(osg::TextureCubeMap::POSITIVE_X,
                                  osgDB::readImageFile( filenames[POS_X] ) );
                cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Y,
                                  osgDB::readImageFile( filenames[NEG_Y] ) );
                cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Y,
                                  osgDB::readImageFile( filenames[POS_Y] ) );
                cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Z,
                                  osgDB::readImageFile( filenames[NEG_Z] ) );
                cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Z,
                                  osgDB::readImageFile( filenames[POS_Z] ) );

                return cubeMap;
            }

            void SkyDomePlugin::update(sReal time_ms)
            {

                // control->motors->setMotorValue(id, value);
            }

            void SkyDomePlugin::receiveData(const data_broker::DataInfo& info,
                                            const data_broker::DataPackage& package,
                                            int id)
            {
                // package.get("force1/x", force);
            }

            void SkyDomePlugin::cfgUpdateProperty(cfg_manager::cfgPropertyStruct _property)
            {
                if(cfgEnableSD.paramId == _property.paramId)
                {
                    if(updateProp)
                    {
                        cfgEnableSD.bValue = _property.bValue;
                        updateProp = false;
                        gui->setMenuActionSelected("../View/Sky dome", cfgEnableSD.bValue);
                        scene->removeChild(posTransform.get());
                        if(materialGroup.valid())
                        {
                            materialGroup->removeChild(meshPos.get());
                        } else
                        {
                            scene->removeChild(meshPos.get());
                        }
                        if(cfgEnableSD.bValue)
                        {
                            scene->addChild(posTransform.get());
                            if(materialGroup.valid())
                            {
                                materialGroup->addChild(meshPos.get());
                            } else
                            {
                                scene->addChild(meshPos.get());
                            }
                        }
                        updateProp = true;
                    }
                } else if(cfgPropPath.paramId == _property.paramId)
                {
                    folder = cfgPropPath.sValue = _property.sValue;
                    LOG_INFO("SkyDome: try to switch to folder: %s", folder.c_str());

                    if(pathExists(resPath + "/Textures/"+folder))
                    {
                        folder = resPath + "/Textures/"+folder;
                    }
                    if(pathExists(folder))
                    {
                        LOG_INFO("SkyDome: switch to folder: %s", folder.c_str());
                        _skyDome->setCubeMap(loadCubeMapTextures().get());
                    }
                }
            }

            void SkyDomePlugin::menuAction (int action, bool checked)
            {
                if(updateProp)
                {
                    cfgEnableSD.bValue = checked;
                    control->cfg->setProperty(cfgEnableSD);
                }
            }

        } // end of namespace SkyDomePlugin
    } // end of namespace plugin
} // end of namespace mars

DESTROY_LIB(mars::plugin::SkyDomePlugin::SkyDomePlugin);
CREATE_LIB(mars::plugin::SkyDomePlugin::SkyDomePlugin);
