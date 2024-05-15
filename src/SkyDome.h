#pragma once
#include "SphereSegment.h"
#include <osg/Program>
#include <osg/Uniform>
#include <osg/TextureCubeMap>

class SkyDome : public SphereSegment
{
public:
    SkyDome( void );
    SkyDome( const SkyDome& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    SkyDome( float radius, unsigned int longSteps, unsigned int latSteps, osg::TextureCubeMap* cubemap );

protected:
    ~SkyDome(void);
	
public:
    void setupStateSet( osg::TextureCubeMap* cubemap );
    void create( float radius, unsigned int latSteps, unsigned int longSteps, osg::TextureCubeMap* cubemap );
	
    inline void setCubeMap( osg::TextureCubeMap* cubemap )
        {
            getOrCreateStateSet()->removeAttribute(cubemap_);
            getOrCreateStateSet()->setTextureAttributeAndModes( 0, cubemap, osg::StateAttribute::ON );
            cubemap_ = cubemap;
        }

private:
    osg::ref_ptr<osg::Program> createShader(void);
    osg::ref_ptr<osg::TextureCubeMap> cubemap_;

};
