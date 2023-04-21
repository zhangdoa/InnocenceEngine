#ifndef CAMERACOMPONENTPROPERTYEDITOR_H
#define CAMERACOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../Engine/Component/CameraComponent.h"

class CameraComponentPropertyEditor : public IComponentPropertyEditor
{
    Q_OBJECT
public:
    CameraComponentPropertyEditor();

    void initialize() override;
    void edit(void* component) override;

    void GetFOV();
    void GetWidthScale();
    void GetHeightScale();
    void GetZNear();
    void GetZFar();
    void GetAperture();
    void GetShutterTime();
    void GetISO();

private:
    ComboLabelText* m_FOV;
    ComboLabelText* m_widthScale;
    ComboLabelText* m_heightScale;
    ComboLabelText* m_zNear;
    ComboLabelText* m_zFar;
    ComboLabelText* m_aperture;
    ComboLabelText* m_shutterTime;
    ComboLabelText* m_ISO;

    Inno::CameraComponent* m_component;

public slots:
    void SetFOV();
    void SetWidthScale();
    void SetHeightScale();
    void SetZNear();
    void SetZFar();
    void SetAperture();
    void SetShutterTime();
    void SetISO();

    void remove() override;
};

#endif // CAMERACOMPONENTPROPERTYEDITOR_H
