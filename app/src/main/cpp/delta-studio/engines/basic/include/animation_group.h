#ifndef DELTA_BASIC_ANIMATION_GROUP_H
#define DELTA_BASIC_ANIMATION_GROUP_H

#include "delta_core.h"

#include "animation_object_controller.h"

namespace dbasic {

    class AnimationGroup : public ysObject {
    public:
        AnimationGroup();
        ~AnimationGroup();

        // update all the animation controllers
        void Update();

        // add an animation controller
        void AddAnimationController(AnimationObjectController *controller);

        // add a sub-group
        AnimationGroup *AddAnimationGroup(const char *groupName);

        // set name
        void SetName(const char *name);

        // set frame
        void SetFrame(int frame);
        void SetTimeOffset(float timeOffset);

    protected:
        // the animation group name
        char m_name[64];

        // list of animation controllers that are part of this animation domain
        ysExpandingArray<AnimationObjectController *, 16> m_animationControllers;

        // child animation groups
        ysDynamicArray<AnimationGroup, 16> m_animationGroups;
    };

} /* namespace dbasic */

#endif /* DELTA_BASIC_ANIMATION_GROUP_H  */
