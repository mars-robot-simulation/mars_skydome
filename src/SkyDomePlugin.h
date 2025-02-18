#pragma once

// set define if you want to extend the gui
#include <mars_interfaces/sim/MarsPluginTemplateGUI.h>
#include <mars_interfaces/MARSDefs.h>
#include <data_broker/ReceiverInterface.h>
#include <cfg_manager/CFGManagerInterface.h>
#include <osg_material_manager/OsgMaterialManager.hpp>
#include <osg_material_manager/MaterialNode.hpp>

#include <string>

#include "SkyDome.h"

namespace mars
{
    namespace plugin
    {
        namespace SkyDomePlugin
        {

            class SkyTransform : public osg::Transform
            {
            public:
                SkyTransform(): s1(1.0), s2(1.0) {}
                virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                       osg::NodeVisitor* nv) const;

                virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                       osg::NodeVisitor* nv) const;
                double s1, s2;
            };

            // inherit from MarsPluginTemplateGUI for extending the gui
            class SkyDomePlugin: public mars::interfaces::MarsPluginTemplateGUI,
                                 public mars::data_broker::ReceiverInterface,
                                 // for gui
                                 public mars::main_gui::MenuInterface,
                                 public mars::cfg_manager::CFGClient
            {

            public:
                SkyDomePlugin(lib_manager::LibManager *theManager);
                ~SkyDomePlugin();

                // LibInterface methods
                int getLibVersion() const
                    { return 1; }
                const std::string getLibName() const
                    { return std::string("mars_skydome"); }
                CREATE_MODULE_INFO();

                // MarsPlugin methods
                void init();
                void reset();
                void update(mars::interfaces::sReal time_ms);

                // DataBrokerReceiver methods
                virtual void receiveData(const data_broker::DataInfo &info,
                                         const data_broker::DataPackage &package,
                                         int callbackParam);
                // CFGClient methods
                virtual void cfgUpdateProperty(cfg_manager::cfgPropertyStruct _property);

                // MenuInterface methods
                void menuAction(int action, bool checked = false);

                // SkyDomePlugin methods

            private:
                cfg_manager::cfgPropertyStruct cfgProp, cfgPropPath, cfgEnableSD;

                osg::ref_ptr<SkyDome> _skyDome;
                osg::ref_ptr<osg::Group> scene;
                osg::ref_ptr<osg_material_manager::MaterialNode> materialGroup;
                osg::ref_ptr<SkyTransform> posTransform, meshPos;
                std::string resPath, folder, meshPath, meshMaterialName;
                double meshScale;
                bool updateProp;
                osg_material_manager::OsgMaterialManager *materialManager;

                osg::ref_ptr<osg::TextureCubeMap> loadCubeMapTextures();
            }; // end of class definition SkyDomePlugin

        } // end of namespace SkyDomePlugin
    } // end of namespace plugin
} // end of namespace mars
