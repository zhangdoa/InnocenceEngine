#ifndef COMBOLABELTEXT_H
#define COMBOLABELTEXT_H

#include <QWidget>
#include <QLineEdit>
#include <QDoubleValidator>

#include "adjustlabel.h"

class ComboLabelText : public QWidget
{
	Q_OBJECT
public:
	explicit ComboLabelText(QWidget *parent = nullptr);

	void Initialize(QString labelText);
	void AlignLabelToTheLeft();
	void AlignLabelToTheRight();

	AdjustLabel* GetLabelWidget();
	QLineEdit* GetTextWidget();

	float GetAsFloat();
	void SetFromFloat(float value);

private:
	AdjustLabel* m_label;
	QLineEdit* m_text;
	QValidator* m_validator;

signals:
	void ValueChanged();

public slots:
	void TextGotEdited();
};

#endif // COMBOLABELTEXT_H
