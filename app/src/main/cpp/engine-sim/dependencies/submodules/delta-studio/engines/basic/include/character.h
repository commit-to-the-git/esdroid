#ifndef DELTA_BASIC_CHARACTER_H
#define DELTA_BASIC_CHARACTER_H

#include "delta_core.h"

#include "animation_group.h"
#include "skeleton.h"
#include "asset_manager.h"
#include "render_skeleton.h"

namespace dbasic {

    class Character : public ysObject {
        friend AssetManager;

    public:
        Character(const char *typeName);
        Character();
        ~Character();

        // update the character
        void Update();

        // set the skeleton being used by this character
        void SetSkeleton(Skeleton *skeleton);

        // set the render skeleton used by this character
        void SetRenderSkeleton(RenderSkeleton *renderSkeleton);

        // set the animation data used by this character
        void SetAnimationData(AnimationExportData *animationData);

        // construct a controller for a bone
        void ConstructBoneController(const char *boneName, AnimationGroup *group);

        // construct a controller for a render node
        void ConstructNodeController(const char *nodeName, AnimationGroup *group);

        // construct the character
        virtual void ConstructCharacter() = 0;

        // get a reference to the main animation controller
        AnimationGroup *GetAnimationController();

        // protected

            // set the asset manager that manages this character
        void SetAssetManager(AssetManager *assetManager);

    protected:
        // reference to the render skeleton
        RenderSkeleton *m_renderSkeleton;

        // references to the skeleton being controlled
        Skeleton *m_skeleton;

        // main top level animation controller
        AnimationGroup m_animationController;

        // animation data like poses motions etc
        AnimationExportData *m_animationData;

        // reference to top level asset manager
        AssetManager *m_assetManager;
    };

} /* namespace dbasic */

#endif /* DELTA_BASIC_CHARACTER_H  */
