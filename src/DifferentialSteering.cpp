#include "DifferentialSteering.h"

DifferentialSteering::DifferentialSteering()
{
    m_fPivYLimit = 32;
    m_leftMotor = 0;
    m_rightMotor = 0;
}

void DifferentialSteering::begin(int fPivYLimit)
{
    m_fPivYLimit = constrain(fPivYLimit, 0, COMPUTERANGE);
}

void DifferentialSteering::computeMotors(int XValue, int YValue)
{
    XValue = constrain(XValue, -COMPUTERANGE, COMPUTERANGE);
    YValue = constrain(YValue, -COMPUTERANGE, COMPUTERANGE);

    float nMotPremixL = 0;
    float nMotPremixR = 0;
    int nPivSpeed = 0;
    float fPivScale = 0;

    if (YValue >= 0)
    {
        nMotPremixL = (XValue >= 0) ? COMPUTERANGE : (COMPUTERANGE + XValue);
        nMotPremixR = (XValue >= 0) ? (COMPUTERANGE - XValue) : COMPUTERANGE;
    }
    else
    {
        nMotPremixL = (XValue >= 0) ? (COMPUTERANGE - XValue) : COMPUTERANGE;
        nMotPremixR = (XValue >= 0) ? COMPUTERANGE : (COMPUTERANGE + XValue);
    }

    nMotPremixL = nMotPremixL * YValue / COMPUTERANGE;
    nMotPremixR = nMotPremixR * YValue / COMPUTERANGE;

    nPivSpeed = XValue;

    if (m_fPivYLimit > 0)
    {
        fPivScale = (abs(YValue) > m_fPivYLimit)
                        ? 0.0f
                        : (1.0f - (float)abs(YValue) / m_fPivYLimit);
    }
    else
    {
        fPivScale = 0.0f;
    }

    m_leftMotor = (1.0f - fPivScale) * nMotPremixL + fPivScale * nPivSpeed;
    m_rightMotor = (1.0f - fPivScale) * nMotPremixR + fPivScale * (-nPivSpeed);

    m_leftMotor = constrain(m_leftMotor, -COMPUTERANGE, COMPUTERANGE);
    m_rightMotor = constrain(m_rightMotor, -COMPUTERANGE, COMPUTERANGE);
}

int DifferentialSteering::computedLeftMotor()
{
    return m_leftMotor;
}

int DifferentialSteering::computedRightMotor()
{
    return m_rightMotor;
}

int DifferentialSteering::getComputeRange()
{
    return COMPUTERANGE;
}

String DifferentialSteering::toString()
{
    return "Pivot threshold: " + String(m_fPivYLimit) +
           " | Left Motor: " + String(m_leftMotor) +
           " | Right Motor: " + String(m_rightMotor);
}
