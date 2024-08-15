#ifndef TRIANGLE_APP_HPP
#define TRIANGLE_APP_HPP

namespace app
{
    class Triangle
    {
    public:
        void run();
    private:
        void init_vulkan();
        void main_loop();
        void cleanup();
    };
}

#endif