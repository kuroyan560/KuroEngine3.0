#pragma once

class CollisionPrimitive
{
public:
	enum SHAPE { SPHERE, MESH };

private:
	const SHAPE shape;
	
protected:
	CollisionPrimitive(const SHAPE& Shape) :shape(Shape) {}

public:
	const SHAPE& GetShape()const { return shape; }
};

