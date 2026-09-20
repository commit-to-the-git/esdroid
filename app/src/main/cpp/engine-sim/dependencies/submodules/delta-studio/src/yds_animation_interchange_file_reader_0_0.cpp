#include "../include/yds_animation_interchange_file_reader_0_0.h"

#include "../include/yds_animation_action.h"

ysAnimationInterchangeFileReader_0_0::ysAnimationInterchangeFileReader_0_0() {
    m_actionCount = 0;
}

ysAnimationInterchangeFileReader_0_0::~ysAnimationInterchangeFileReader_0_0() {
    /* void */
}

/*
yserror ysanimationinterchangefilereader_0_0::openconst char *fname {
    YDS_ERROR_DECLARE("Open");

    m_file.openfname std::ios::binary | std::ios::in | std::ios::out
    if m_file.is_open return yds_error_return_msgyserror::yds_could_not_open_file fname

    idheader idheader
    m_file.readchar *&idheader sizeofidheader

    if idheader.magicnumber != magic_number {
        m_file.close();
        return yds_error_returnyserror::yds_invalid_file_type
    }

    if idheader.minorversion != minor_version || idheader.majorversion != major_version {
        m_file.close();
        return yds_error_returnyserror::yds_unsupported_file_version
    }

    m_compilationstatus = idheader.compilationstatus == 0x0
    m_toolid = idheader.editorid

    fileheader fileheader
    m_file.readchar *&fileheader sizeoffileheader

    m_actioncount = fileheader.actioncount

    return yds_error_returnyserror::none
}

yserror ysanimationinterchangefile0_0::close {
    YDS_ERROR_DECLARE("Close");

    if m_file.is_open m_file.close

    m_majorversion = -1
    m_minorversion = -1

    return yds_error_returnyserror::none
}*/

ysAnimationCurve::CurveType ysAnimationInterchangeFileReader_0_0::InterpretCurveType(unsigned int curveType) {
    switch (curveType) {
    case 0x1: return ysAnimationCurve::CurveType::LocationVec;
    case 0x2: return ysAnimationCurve::CurveType::RotationQuat;
    case 0x3: return ysAnimationCurve::CurveType::LocationX;
    case 0x4: return ysAnimationCurve::CurveType::LocationY;
    case 0x5: return ysAnimationCurve::CurveType::LocationZ;
    case 0x6: return ysAnimationCurve::CurveType::RotationQuatW;
    case 0x7: return ysAnimationCurve::CurveType::RotationQuatX;
    case 0x8: return ysAnimationCurve::CurveType::RotationQuatY;
    case 0x9: return ysAnimationCurve::CurveType::RotationQuatZ;
    case 0x0:
    default: return ysAnimationCurve::CurveType::Undefined;
    }
}

ysError ysAnimationInterchangeFileReader_0_0::ReadHeader(std::fstream &f) {
    YDS_ERROR_DECLARE("ReaderHeader");

    FileHeader fileHeader;
    f.read((char *)&fileHeader, sizeof(FileHeader));

    m_actionCount = fileHeader.ActionCount;

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysAnimationInterchangeFileReader_0_0::ReadAction(std::fstream &f, ysAnimationAction *action) {
    YDS_ERROR_DECLARE("ReadAction");

    ActionHeader actionHeader;
    f.read((char *)&actionHeader, sizeof(ActionHeader));

    action->SetName(actionHeader.Name);
    
    int curveCount = actionHeader.CurveCount;
    for (int i = 0; i < curveCount; ++i) {
        CurveHeader curveHeader;
        f.read((char *)&curveHeader, sizeof(CurveHeader));

        ysAnimationCurve *newCurve = action->NewCurve(curveHeader.TargetBone);
        newCurve->SetCurveType(InterpretCurveType(curveHeader.CurveType));
        
        int keyframeCount = curveHeader.KeyframeCount;
        for (int i = 0; i < keyframeCount; ++i) {
            Keyframe keyframe;
            f.read((char *)&keyframe, sizeof(Keyframe));

            ysAnimationCurve::CurveHandle handle;
            handle.mode = 
                ysAnimationCurve::CurveHandle::InterpolationMode::Linear;
            handle.s = keyframe.Timestamp;
            handle.v = keyframe.Value;

            // bezier interpolation is not supported in this file version
            handle.l_handle_x = handle.l_handle_y = 0.0f;
            handle.r_handle_x = handle.r_handle_y = 0.0f;

            newCurve->AddSamplePoint(handle);
        }
    }

    return YDS_ERROR_RETURN(ysError::None);
}
