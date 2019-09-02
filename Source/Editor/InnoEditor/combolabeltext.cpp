#include "combolabeltext.h"

ComboLabelText::ComboLabelText(QWidget *parent) : QWidget(parent)
{
	m_label = nullptr;
	m_text = nullptr;
}

void ComboLabelText::Initialize(QString labelText)
{
	m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
	m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

	m_text = new QLineEdit();
	m_text->setValidator(m_validator);

	m_label = new AdjustLabel();
	m_label->setText(labelText);
	m_label->setAlignment(Qt::AlignRight);
	m_label->Initialize(m_text);

	connect(m_label, SIGNAL(Adjusted()), this, SLOT(TextGotEdited()));
	connect(m_text, SIGNAL(textEdited(QString)), this, SLOT(TextGotEdited()));
}

void ComboLabelText::AlignLabelToTheLeft()
{
	m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void ComboLabelText::AlignLabelToTheRight()
{
	m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

AdjustLabel*ComboLabelText::GetLabelWidget()
{
	return m_label;
}

QLineEdit*ComboLabelText::GetTextWidget()
{
	return m_text;
}

float ComboLabelText::GetAsFloat()
{
	return m_text->text().toFloat();
}

void ComboLabelText::SetFromFloat(float value)
{
	m_text->setText(QString::number((double)value));
}

void ComboLabelText::TextGotEdited()
{
	emit ValueChanged();
}