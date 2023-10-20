#pragma once

#include "Nodes/Widgets/Poly.h"

class PolyRect : public Poly
{
public:

    DECLARE_NODE(PolyRect, Poly);

    PolyRect();
    virtual ~PolyRect();

    virtual void Tick(float deltaTime) override;
};
