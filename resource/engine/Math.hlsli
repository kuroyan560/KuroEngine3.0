static const float PI = 3.14159265359f;

//イージング
float Easing_Quad_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return max * t * t + min;
}
float2 Easing_Quad_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quad_In(t, totaltime, min.x, max.x), 
    Easing_Quad_In(t, totaltime, min.y, max.y));
}
float3 Easing_Quad_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quad_In(t, totaltime, min.x, max.x),
    Easing_Quad_In(t, totaltime, min.y, max.y),
    Easing_Quad_In(t, totaltime, min.z, max.z));
}
float Easing_Quad_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return -max * t * (t - 2) + min;
}
float2 Easing_Quad_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quad_Out(t, totaltime, min.x, max.x),
    Easing_Quad_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Quad_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quad_Out(t, totaltime, min.x, max.x),
    Easing_Quad_Out(t, totaltime, min.y, max.y),
    Easing_Quad_Out(t, totaltime, min.z, max.z));
}
float Easing_Qurd_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;
    if (t < 1)
        return max / 2 * t * t + min;

    t = t - 1;
    return -max / 2 * (t * (t - 2) - 1) + min;
}
float2 Easing_Qurd_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Qurd_InOut(t, totaltime, min.x, max.x),
    Easing_Qurd_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Qurd_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Qurd_InOut(t, totaltime, min.x, max.x),
    Easing_Qurd_InOut(t, totaltime, min.y, max.y),
    Easing_Qurd_InOut(t, totaltime, min.z, max.z));
}

float Easing_Cubic_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return max * t * t * t + min;
}
float2 Easing_Cubic_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Cubic_In(t, totaltime, min.x, max.x),
    Easing_Cubic_In(t, totaltime, min.y, max.y));
}
float3 Easing_Cubic_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Cubic_In(t, totaltime, min.x, max.x),
    Easing_Cubic_In(t, totaltime, min.y, max.y),
    Easing_Cubic_In(t, totaltime, min.z, max.z));
}
float Easing_Cubic_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t = t / totaltime - 1;
    return max * (t * t * t + 1) + min;
}
float2 Easing_Cubic_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Cubic_Out(t, totaltime, min.x, max.x),
    Easing_Cubic_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Cubic_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Cubic_Out(t, totaltime, min.x, max.x),
    Easing_Cubic_Out(t, totaltime, min.y, max.y),
    Easing_Cubic_Out(t, totaltime, min.z, max.z));
}
float Easing_Cubic_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;
    if (t < 1)
        return max / 2 * t * t * t + min;

    t = t - 2;
    return max / 2 * (t * t * t + 2) + min;
}
float2 Easing_Cubic_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Cubic_InOut(t, totaltime, min.x, max.x),
    Easing_Cubic_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Cubic_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Cubic_InOut(t, totaltime, min.x, max.x),
    Easing_Cubic_InOut(t, totaltime, min.y, max.y),
    Easing_Cubic_InOut(t, totaltime, min.z, max.z));
}

float Easing_Quart_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return max * t * t * t * t + min;
}
float2 Easing_Quart_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quart_In(t, totaltime, min.x, max.x),
    Easing_Quart_In(t, totaltime, min.y, max.y));
}
float3 Easing_Quart_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quart_In(t, totaltime, min.x, max.x),
    Easing_Quart_In(t, totaltime, min.y, max.y),
    Easing_Quart_In(t, totaltime, min.z, max.z));
}
float Easing_Quart_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t = t / totaltime - 1;
    return -max * (t * t * t * t - 1) + min;
}
float2 Easing_Quart_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quart_Out(t, totaltime, min.x, max.x),
    Easing_Quart_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Quart_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quart_Out(t, totaltime, min.x, max.x),
    Easing_Quart_Out(t, totaltime, min.y, max.y),
    Easing_Quart_Out(t, totaltime, min.z, max.z));
}
float Easing_Quart_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;
    if (t < 1)
        return max / 2 * t * t * t * t + min;

    t = t - 2;
    return -max / 2 * (t * t * t * t - 2) + min;
}
float2 Easing_Quart_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quart_InOut(t, totaltime, min.x, max.x),
    Easing_Quart_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Quart_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quart_InOut(t, totaltime, min.x, max.x),
    Easing_Quart_InOut(t, totaltime, min.y, max.y),
    Easing_Quart_InOut(t, totaltime, min.z, max.z));
}

float Easing_Quint_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return max * t * t * t * t * t + min;
}
float2 Easing_Quint_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quint_In(t, totaltime, min.x, max.x),
    Easing_Quint_In(t, totaltime, min.y, max.y));
}
float3 Easing_Quint_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quint_In(t, totaltime, min.x, max.x),
    Easing_Quint_In(t, totaltime, min.y, max.y),
    Easing_Quint_In(t, totaltime, min.z, max.z));
}
float Easing_Quint_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t = t / totaltime - 1;
    return max * (t * t * t * t * t + 1) + min;
}
float2 Easing_Quint_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quint_Out(t, totaltime, min.x, max.x),
    Easing_Quint_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Quint_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quint_Out(t, totaltime, min.x, max.x),
    Easing_Quint_Out(t, totaltime, min.y, max.y),
    Easing_Quint_Out(t, totaltime, min.z, max.z));
}
float Easing_Quint_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;
    if (t < 1)
        return max / 2 * t * t * t * t * t + min;

    t = t - 2;
    return max / 2 * (t * t * t * t * t + 2) + min;
}
float2 Easing_Quint_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Quint_InOut(t, totaltime, min.x, max.x),
    Easing_Quint_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Quint_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Quint_InOut(t, totaltime, min.x, max.x),
    Easing_Quint_InOut(t, totaltime, min.y, max.y),
    Easing_Quint_InOut(t, totaltime, min.z, max.z));
}

float Easing_Sine_In(float t, float totaltime, float min, float max)
{
    max -= min;
    return -max * cos(t * (PI * 90 / 180) / totaltime) + max + min;
}
float2 Easing_Sine_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Sine_In(t, totaltime, min.x, max.x),
    Easing_Sine_In(t, totaltime, min.y, max.y));
}
float3 Easing_Sine_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Sine_In(t, totaltime, min.x, max.x),
    Easing_Sine_In(t, totaltime, min.y, max.y),
    Easing_Sine_In(t, totaltime, min.z, max.z));
}
float Easing_Sine_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    return max * sin(t * (PI * 90 / 180) / totaltime) + min;
}
float2 Easing_Sine_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Sine_Out(t, totaltime, min.x, max.x),
    Easing_Sine_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Sine_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Sine_Out(t, totaltime, min.x, max.x),
    Easing_Sine_Out(t, totaltime, min.y, max.y),
    Easing_Sine_Out(t, totaltime, min.z, max.z));
}
float Easing_Sine_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    return -max / 2 * (cos(t * PI / totaltime) - 1) + min;
}
float2 Easing_Sine_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Sine_InOut(t, totaltime, min.x, max.x),
    Easing_Sine_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Sine_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Sine_InOut(t, totaltime, min.x, max.x),
    Easing_Sine_InOut(t, totaltime, min.y, max.y),
    Easing_Sine_InOut(t, totaltime, min.z, max.z));
}

float Easing_Exp_In(float t, float totaltime, float min, float max)
{
    max -= min;
    return t == 0.0 ? min : max * pow(2, 10 * (t / totaltime - 1)) + min;
}
float2 Easing_Exp_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Exp_In(t, totaltime, min.x, max.x),
    Easing_Exp_In(t, totaltime, min.y, max.y));
}
float3 Easing_Exp_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Exp_In(t, totaltime, min.x, max.x),
    Easing_Exp_In(t, totaltime, min.y, max.y),
    Easing_Exp_In(t, totaltime, min.z, max.z));
}
float Easing_Exp_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    return t == totaltime ? max + min : max * (-pow(2, -10 * t / totaltime) + 1) + min;
}
float2 Easing_Exp_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Exp_Out(t, totaltime, min.x, max.x),
    Easing_Exp_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Exp_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Exp_Out(t, totaltime, min.x, max.x),
    Easing_Exp_Out(t, totaltime, min.y, max.y),
    Easing_Exp_Out(t, totaltime, min.z, max.z));
}
float Easing_Exp_InOut(float t, float totaltime, float min, float max)
{
    if (t == 0.0f)
        return min;
    if (t == totaltime)
        return max;
    max -= min;
    t /= totaltime / 2;

    if (t < 1)
        return max / 2 * pow(2, 10 * (t - 1)) + min;

    t = t - 1;
    return max / 2 * (-pow(2, -10 * t) + 2) + min;
}
float2 Easing_Exp_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Exp_InOut(t, totaltime, min.x, max.x),
    Easing_Exp_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Exp_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Exp_InOut(t, totaltime, min.x, max.x),
    Easing_Exp_InOut(t, totaltime, min.y, max.y),
    Easing_Exp_InOut(t, totaltime, min.z, max.z));
}


float Easing_Circ_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;
    return -max * (sqrt(1 - t * t) - 1) + min;
}
float2 Easing_Circ_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Circ_In(t, totaltime, min.x, max.x),
    Easing_Circ_In(t, totaltime, min.y, max.y));
}
float3 Easing_Circ_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Circ_In(t, totaltime, min.x, max.x),
    Easing_Circ_In(t, totaltime, min.y, max.y),
    Easing_Circ_In(t, totaltime, min.z, max.z));
}
float Easing_Circ_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t = t / totaltime - 1;
    return max * sqrt(1 - t * t) + min;
}
float2 Easing_Circ_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Circ_Out(t, totaltime, min.x, max.x),
    Easing_Circ_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Circ_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Circ_Out(t, totaltime, min.x, max.x),
    Easing_Circ_Out(t, totaltime, min.y, max.y),
    Easing_Circ_Out(t, totaltime, min.z, max.z));
}
float Easing_Circ_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;
    if (t < 1)
        return -max / 2 * (sqrt(1 - t * t) - 1) + min;

    t = t - 2;
    return max / 2 * (sqrt(1 - t * t) + 1) + min;
}
float2 Easing_Circ_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Circ_InOut(t, totaltime, min.x, max.x),
    Easing_Circ_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Circ_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Circ_InOut(t, totaltime, min.x, max.x),
    Easing_Circ_InOut(t, totaltime, min.y, max.y),
    Easing_Circ_InOut(t, totaltime, min.z, max.z));
}


float Easing_Elastic_In(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;

    float s = 1.70158f;
    float p = totaltime * 0.3f;
    float a = max;

    if (t == 0)
        return min;
    if (t == 1)
        return min + max;

    if (a < abs(max))
    {
        a = max;
        s = p / 4;
    }
    else
    {
        s = p / (2 * PI) * asin(max / a);
    }

    t = t - 1;
    return -(a * pow(2, 10 * t) * sin((t * totaltime - s) * (2 * PI) / p)) + min;
}
float2 Easing_Elastic_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Elastic_In(t, totaltime, min.x, max.x),
    Easing_Elastic_In(t, totaltime, min.y, max.y));
}
float3 Easing_Elastic_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Elastic_In(t, totaltime, min.x, max.x),
    Easing_Elastic_In(t, totaltime, min.y, max.y),
    Easing_Elastic_In(t, totaltime, min.z, max.z));
}
float Easing_Elastic_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;

    float s = 1.70158f;
    float p = totaltime * 0.3f;;
    float a = max;

    if (t == 0)
        return min;
    if (t == 1)
        return min + max;

    if (a < abs(max))
    {
        a = max;
        s = p / 4;
    }
    else
    {
        s = p / (2 * PI) * asin(max / a);
    }

    return a * pow(2, -10 * t) * sin((t * totaltime - s) * (2 * PI) / p) + max + min;
}
float2 Easing_Elastic_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Elastic_Out(t, totaltime, min.x, max.x),
    Easing_Elastic_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Elastic_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Elastic_Out(t, totaltime, min.x, max.x),
    Easing_Elastic_Out(t, totaltime, min.y, max.y),
    Easing_Elastic_Out(t, totaltime, min.z, max.z));
}
float Easing_Elastic_InOut(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime / 2;

    float s = 1.70158f;
    float p = totaltime * (0.3f * 1.5f);
    float a = max;

    if (t == 0)
        return min;
    if (t == 2)
        return min + max;

    if (a < abs(max))
    {
        a = max;
        s = p / 4;
    }
    else
    {
        s = p / (2 * PI) * asin(max / a);
    }

    if (t < 1)
    {
        return -0.5f * (a * pow(2, 10 * (t -= 1)) * sin((t * totaltime - s) * (2 * PI) / p)) + min;
    }

    t = t - 1;
    return a * pow(2, -10 * t) * sin((t * totaltime - s) * (2 * PI) / p) * 0.5f + max + min;
}
float2 Easing_Elastic_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Elastic_InOut(t, totaltime, min.x, max.x),
    Easing_Elastic_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Elastic_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Elastic_InOut(t, totaltime, min.x, max.x),
    Easing_Elastic_InOut(t, totaltime, min.y, max.y),
    Easing_Elastic_InOut(t, totaltime, min.z, max.z));
}

float Easing_Back_In(float t, float totaltime, float min, float max)
{
    float s = 1.70158f;
    max -= min;
    t /= totaltime;
    return max * t * t * ((s + 1) * t - s) + min;
}
float2 Easing_Back_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Back_In(t, totaltime, min.x, max.x),
    Easing_Back_In(t, totaltime, min.y, max.y));
}
float3 Easing_Back_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Back_In(t, totaltime, min.x, max.x),
    Easing_Back_In(t, totaltime, min.y, max.y),
    Easing_Back_In(t, totaltime, min.z, max.z));
}
float Easing_Back_Out(float t, float totaltime, float min, float max)
{
    float s = 1.70158f;
    max -= min;
    t = t / totaltime - 1;
    return max * (t * t * ((s + 1) * t + s) + 1) + min;
}
float2 Easing_Back_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Back_Out(t, totaltime, min.x, max.x),
    Easing_Back_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Back_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Back_Out(t, totaltime, min.x, max.x),
    Easing_Back_Out(t, totaltime, min.y, max.y),
    Easing_Back_Out(t, totaltime, min.z, max.z));
}
float Easing_Back_InOut(float t, float totaltime, float min, float max)
{
    float s = 1.70158f;
    max -= min;
    s *= 1.525f;
    t /= totaltime / 2;
    if (t < 1)
        return max / 2 * (t * t * ((s + 1) * t - s)) + min;

    t = t - 2;
    return max / 2 * (t * t * ((s + 1) * t + s) + 2) + min;
}
float2 Easing_Back_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Back_InOut(t, totaltime, min.x, max.x),
    Easing_Back_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Back_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Back_InOut(t, totaltime, min.x, max.x),
    Easing_Back_InOut(t, totaltime, min.y, max.y),
    Easing_Back_InOut(t, totaltime, min.z, max.z));
}

float Easing_Bounce_Out(float t, float totaltime, float min, float max)
{
    max -= min;
    t /= totaltime;

    if (t < 1.0f / 2.75f)
    {
        return max * (7.5625f * t * t) + min;
    }
    else if (t < 2.0f / 2.75f)
    {
        t -= 1.5f / 2.75f;
        return max * (7.5625f * t * t + 0.75f) + min;
    }
    else if (t < 2.5f / 2.75f)
    {
        t -= 2.25f / 2.75f;
        return max * (7.5625f * t * t + 0.9375f) + min;
    }
    else
    {
        t -= 2.625f / 2.75f;
        return max * (7.5625f * t * t + 0.984375f) + min;
    }
}
float2 Easing_Bounce_Out(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Bounce_Out(t, totaltime, min.x, max.x),
    Easing_Bounce_Out(t, totaltime, min.y, max.y));
}
float3 Easing_Bounce_Out(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Bounce_Out(t, totaltime, min.x, max.x),
    Easing_Bounce_Out(t, totaltime, min.y, max.y),
    Easing_Bounce_Out(t, totaltime, min.z, max.z));
}
float Easing_Bounce_In(float t, float totaltime, float min, float max)
{
    max -= min;
    return max - Easing_Bounce_Out(totaltime - t, totaltime, 0, max) + min;
}
float2 Easing_Bounce_In(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Bounce_In(t, totaltime, min.x, max.x),
    Easing_Bounce_In(t, totaltime, min.y, max.y));
}
float3 Easing_Bounce_In(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Bounce_In(t, totaltime, min.x, max.x),
    Easing_Bounce_In(t, totaltime, min.y, max.y),
    Easing_Bounce_In(t, totaltime, min.z, max.z));
}
float Easing_Bounce_InOut(float t, float totaltime, float min, float max)
{
    if (t < totaltime / 2)
    {
        return Easing_Bounce_In(t * 2, totaltime, 0, max - min) * 0.5f + min;
    }
    else
    {
        return Easing_Bounce_Out(t * 2 - totaltime, totaltime, 0, max - min) * 0.5f + min + (max - min) * 0.5f;
    }
}
float2 Easing_Bounce_InOut(float t, float totaltime, float2 min, float2 max)
{
    return float2(
    Easing_Bounce_InOut(t, totaltime, min.x, max.x),
    Easing_Bounce_InOut(t, totaltime, min.y, max.y));
}
float3 Easing_Bounce_InOut(float t, float totaltime, float3 min, float3 max)
{
    return float3(
    Easing_Bounce_InOut(t, totaltime, min.x, max.x),
    Easing_Bounce_InOut(t, totaltime, min.y, max.y),
    Easing_Bounce_InOut(t, totaltime, min.z, max.z));
}

//ポスタライズ
float Posterize(float input,float step)
{
    return floor(input / (1.0f / step)) * (1.0f / step);
}

float2 Posterize(float2 input, float step)
{
    return float2(Posterize(input.x, step), Posterize(input.y, step));
};

float3 Posterize(float3 input,float step)
{
    float2 xy = Posterize(input.xy, step);
    return float3(xy.x, xy.y, Posterize(input.z, step));
}

float4 Posterize(float4 input,float step)
{
    float3 xyz = Posterize(input.xyz, step);
    return float4(xyz.x, xyz.y, xyz.z, Posterize(input.w, step));
}