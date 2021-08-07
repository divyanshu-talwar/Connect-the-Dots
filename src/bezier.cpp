#if defined(NANOGUI_GLAD)
    #if defined(NANOGUI_SHARED) && !defined(GLAD_GLAPI_EXPORT)
        #define GLAD_GLAPI_EXPORT
    #endif

    #include <glad/glad.h>
#else
    #if defined(__APPLE__)
        #define GLFW_INCLUDE_GLCOREARB
    #else
        #define GL_GLEXT_PROTOTYPES
    #endif
#endif

#include <GLFW/glfw3.h>

#include <nanogui/nanogui.h>
#include <iostream>
#include <math.h>

using namespace nanogui;

enum test_enum {
    Item1 = 0,
    Item2,
    Item3
};

bool bvar = true;
int ivar = 12345678;
double dvar = 3.1415926;
float fvar = (float)dvar;
std::string strval = "A string";
test_enum enumval = Item2;

// Variables defined for the ease of execution of this program.
bool add_points = true;
bool first_point = true;
bool bezier_selected = false;
std::vector<GLfloat> points;
std::vector<GLfloat> control_points;
std::vector<GLfloat> draw_points;
int selected_point_index = -1;

Color colval(1.0f, 1.0f, 0.4f, 1.f);

Screen *screen = nullptr;

double distance(std::pair<double, double> a, std::pair<double, double> b){
    return sqrt(pow((a.first - b.first), 2) + pow((a.second - b.second), 2));
}

std::pair<GLfloat, GLfloat> find_next_control_point(std::pair<GLfloat, GLfloat> prev_control, std::pair<GLfloat, GLfloat> this_anchor){
    std::pair<GLfloat, GLfloat> next_control_point;
    double theta = atan2(double(this_anchor.second - prev_control.second), double(this_anchor.first - prev_control.first));
    double r = distance(this_anchor, prev_control);
    next_control_point.first = this_anchor.first + r*cos(theta);
    next_control_point.second = this_anchor.second + r*sin(theta);
    return next_control_point;
}

std::pair<GLfloat, GLfloat> generate_random_point(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat factor){
    std::pair<GLfloat, GLfloat> control_point;
    control_point.first = x1 + (factor/3)*(x2 - x1);
    control_point.second = std::max(y2, y1) + (factor/3)*(y2 - y1);;
    return control_point;
}


std::pair<GLfloat, GLfloat> cubic_bezier_curve(std::pair<GLfloat, GLfloat> P0, std::pair<GLfloat, GLfloat> P1, std::pair<GLfloat, GLfloat> P2, std::pair<GLfloat, GLfloat> P3, GLfloat t){
    return std::make_pair((pow((1 - t), 3)*P0.first + 3*pow((1-t), 2)*t*P1.first + 3*pow(t, 2)*(1-t)*P2.first + pow(t, 3)*P3.first), (pow((1 - t), 3)*P0.second + 3*pow((1-t), 2)*t*P1.second+ 3*pow(t, 2)*(1-t)*P2.second + pow(t, 3)*P3.second));
}

void populate_draw_points(std::pair<GLfloat, GLfloat> start, std::pair<GLfloat, GLfloat> c1, std::pair<GLfloat, GLfloat> c2, std::pair<GLfloat, GLfloat> end){
    int tot = 36;
    for(float i = 0; i< 1; i+= float(1.0/tot)){
        std::pair<GLfloat, GLfloat> next_point = cubic_bezier_curve(start, c1, c2, end, i);
        draw_points.insert(draw_points.end(), {next_point.first, next_point.second, 0.0f});
    }
}

void modify_draw_points(std::pair<GLfloat, GLfloat> start, std::pair<GLfloat, GLfloat> c1, std::pair<GLfloat, GLfloat> c2, std::pair<GLfloat, GLfloat> end, int postition){
    int tot = 36;
    for(float i = 0; i< 1; i+= float(1.0/tot)){
        std::pair<GLfloat, GLfloat> next_point = cubic_bezier_curve(start, c1, c2, end, i);
        draw_points[postition*3*tot + i*3*tot] = next_point.first;
        draw_points[postition*3*tot + i*3*tot + 1] = next_point.second;
        draw_points[postition*3*tot + i*3*tot + 2] = 0.0f;
    }
}

void find_selected_point_index(double x, double y){
    double min = 10000;
    int min_ind = 0;
    int i;
    for(i = 0; i<points.size() ; i+=3){
        double dist = distance(std::make_pair(x, y), std::make_pair(points[i], points[i+1]));
        if(min > dist){
            min = dist;
            min_ind = i;
        }
    }
    selected_point_index = min_ind+3;
}

void clear_all_callback(){
    std::cout << "[info-selection] Cleared.\n";
    points.clear();
    control_points.clear();
    draw_points.clear();
    add_points = true;
    first_point = true;
    selected_point_index = -1;
}

void polybezier_callback(){
    std::cout << "[info-selection] Bezier Curve.\n";
    points.clear();
    control_points.clear();
    draw_points.clear();
    add_points = true;
    first_point = true;
    bezier_selected = true;
    selected_point_index = -1;
}

void polyline_callback(){
    std::cout << "[info-selection] Poly-line.\n";
    points.clear();
    control_points.clear();
    draw_points.clear();
    add_points = true;
    first_point = true;
    bezier_selected = false;
    selected_point_index = -1;
}


void glfw_mouse_click_callback( GLFWwindow *window, int button, int action, int mods ) {
    if(!bezier_selected){
        if ( GLFW_PRESS == action) {
            if(add_points){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                if(first_point){
                    first_point = false;
                    points.insert(points.end(), {x, y, 0.0f});
                    points.insert(points.end(), {x, y, 0.0f});
                }
                else{
                    points.erase(points.begin()+points.size()-3, points.begin()+points.size());
                    points.insert(points.end(), {x, y, 0.0f});
                    points.insert(points.end(), {x, y, 0.0f});
                }
            }
            if(GLFW_MOUSE_BUTTON_RIGHT == button){
                std::cout<<"[info-input] Right mouse button clicked."<<"\n";
                points.erase(points.begin()+points.size()-3, points.begin()+points.size());
                add_points = false;
            }
            if(!add_points && selected_point_index == -1 && GLFW_MOUSE_BUTTON_RIGHT != button){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                find_selected_point_index(x, y);
                std::cout<<"[info] Dragging point number : "<<selected_point_index/3<<"\n";
            }
        }
        if ( GLFW_RELEASE == action) {
            if(!add_points && selected_point_index != -1 && GLFW_MOUSE_BUTTON_RIGHT != button){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                points[selected_point_index - 3] = x;
                points[selected_point_index - 2] = y;
                selected_point_index = -1;
            }   
        }
    }
    if(bezier_selected){
        if ( GLFW_PRESS == action) {
            if(add_points){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                if(!first_point && points.size() > 3){
                    std::pair<GLfloat, GLfloat> c1 = find_next_control_point(std::make_pair(control_points[control_points.size() - 3], control_points[control_points.size() - 2]), std::make_pair(points[points.size() - 3], points[points.size() - 2]));
                    control_points.push_back(c1.first);
                    control_points.push_back(c1.second);
                    control_points.push_back(0.0f);
                    std::pair<GLfloat, GLfloat> c2 = generate_random_point(points[points.size() - 3], points[points.size() - 2], x, y, 2.0);
                    control_points.push_back(c2.first);
                    control_points.push_back(c2.second);
                    control_points.push_back(0.0f);
                    populate_draw_points(std::make_pair(points[points.size() - 3], points[points.size() - 2]), c1, c2, std::make_pair(x, y));
                    points.insert(points.end(), {x, y, 0.0f});
                }
                if(points.size() == 3){
                    points.insert(points.end(), {x, y, 0.0f});
                    std::pair<GLfloat, GLfloat> c1 = generate_random_point(points[0], points[1], x, y, 1.0);
                    control_points.push_back(c1.first);
                    control_points.push_back(c1.second);
                    control_points.push_back(0.0f);
                    std::pair<GLfloat, GLfloat> c2 = generate_random_point(points[0], points[1], x, y, 2.0);
                    control_points.push_back(c2.first);
                    control_points.push_back(c2.second);
                    control_points.push_back(0.0f);
                    populate_draw_points(std::make_pair(points[0], points[1]), c1, c2, std::make_pair(x, y));
                }
                if(first_point){
                    first_point = false;
                    points.insert(points.end(), {x, y, 0.0f});
                }
            }
            if(GLFW_MOUSE_BUTTON_RIGHT == button){
                std::cout<<"[info-input] Right mouse button clicked."<<"\n";
                add_points = false;
            }
            if(!add_points && selected_point_index == -1 && GLFW_MOUSE_BUTTON_RIGHT != button){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                find_selected_point_index(x, y);
                std::cout<<"[info] Dragging point number : "<<selected_point_index/3<<"\n";
            }
        }
        if ( GLFW_RELEASE == action) {
            if(!add_points && selected_point_index != -1 && GLFW_MOUSE_BUTTON_RIGHT != button){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                std::cout << "x : "<< xpos << ",  y : " << ypos << "\n";
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                double del_x = x - points[selected_point_index - 3];
                double del_y = y - points[selected_point_index - 2];
                points[selected_point_index - 3] = x;
                points[selected_point_index - 2] = y;
                if(selected_point_index == 3){
                    control_points[0] += del_x;
                    control_points[1] += del_y;

                    std::pair<GLfloat, GLfloat> c1 = std::make_pair(control_points[0], control_points[1]);
                    std::pair<GLfloat, GLfloat> c2 = std::make_pair(control_points[3], control_points[4]);

                    modify_draw_points(std::make_pair(points[0], points[1]), c1, c2, std::make_pair(points[3], points[4]), 0);
                }
                else if(selected_point_index == points.size()){
                    control_points[control_points.size() - 3] += del_x;
                    control_points[control_points.size() - 2] += del_y;

                    std::pair<GLfloat, GLfloat> c1 = std::make_pair(control_points[control_points.size() - 6], control_points[control_points.size() - 5]);
                    std::pair<GLfloat, GLfloat> c2 = std::make_pair(control_points[control_points.size() - 3], control_points[control_points.size() - 2]);

                    modify_draw_points(std::make_pair(points[points.size() - 6], points[points.size() - 5]), c1, c2, std::make_pair(points[points.size() - 3], points[points.size() - 2]), (selected_point_index/3 -2));
                }
                else{
                    int number_of_control_pts_till_now = (selected_point_index/3 -1)*2;

                    control_points[(number_of_control_pts_till_now - 1)*3] += del_x;
                    control_points[number_of_control_pts_till_now*3] += del_x;
                    control_points[(number_of_control_pts_till_now - 1)*3 + 1] += del_y;
                    control_points[number_of_control_pts_till_now*3 + 1] += del_y;

                    std::pair<GLfloat, GLfloat> c1 = std::make_pair(control_points[(number_of_control_pts_till_now - 2)*3], control_points[(number_of_control_pts_till_now - 2)*3 + 1]);
                    std::pair<GLfloat, GLfloat> c2 = std::make_pair(control_points[(number_of_control_pts_till_now - 1)*3], control_points[(number_of_control_pts_till_now - 1)*3 + 1]);
                    modify_draw_points(std::make_pair(points[selected_point_index - 6], points[selected_point_index - 5]), c1, c2, std::make_pair(points[selected_point_index - 3], points[selected_point_index - 2]), selected_point_index/3 - 2);
                    c1 = std::make_pair(control_points[(number_of_control_pts_till_now)*3], control_points[(number_of_control_pts_till_now)*3 + 1]);
                    c2 = std::make_pair(control_points[(number_of_control_pts_till_now + 1)*3], control_points[(number_of_control_pts_till_now + 1)*3 + 1]);
                    modify_draw_points(std::make_pair(points[selected_point_index - 3], points[selected_point_index - 2]), c1, c2, std::make_pair(points[selected_point_index], points[selected_point_index+1]), selected_point_index/3 - 1);    
                }
                selected_point_index = -1;
            }   
        }
    }

}


int main(int /* argc */, char ** /* argv */) {
    GLFWwindow *window = NULL;
    const GLubyte *renderer;
    const GLubyte *version;
    GLuint vao;
    GLuint vbo;
    
    /* these are the strings of code for the shaders
    the vertex shader positions each vertex point */
    const char *vertex_shader = "#version 410\n"
        "in vec3 vp;"
        "void main () {"
        "  gl_Position = vec4 (vp, 1.0);"
        "}";
    /* the fragment shader colours each fragment (pixel-sized area of the
    triangle) */
    const char *fragment_shader = "#version 410\n"
        "out vec4 frag_colour;"
        "void main () {"
        "frag_colour = vec4 (0.0, 0.6, 0.2, 1.0);"
        "}";
    GLuint vert_shader, frag_shader;
    GLuint shader_programme;

    if ( !glfwInit() ) {
        fprintf( stderr, "ERROR: could not start GLFW3\n" );
        return 1;
    }

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    window = glfwCreateWindow( 640, 480, "Bezier Curve", NULL, NULL );
    if ( !window ) {
        fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent( window );
    glClearColor(1.0, 1.0, 0.4, 1.0);

    screen = new Screen();
    screen->initialize(window, true);

    // Create nanogui gui
    FormHelper *gui = new FormHelper(screen);
    ref<Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Menu");
    
    gui->addButton("1.  Polyline       ", []() { polyline_callback(); })->setTooltip("Button");;
    gui->addButton("2.  Polybezier  ", []() { polybezier_callback(); })->setTooltip("Button");;
    gui->addButton("3.  Clear All      ", []() { clear_all_callback(); })->setTooltip("Button");;

    screen->setVisible(true);
    screen->performLayout();
    // nanoguiWindow->center();

    renderer = glGetString( GL_RENDERER ); /* get renderer string */
    version = glGetString( GL_VERSION );     /* version as a string */
    printf( "Renderer: %s\n", renderer );
    printf( "OpenGL version supported %s\n", version );

    glEnable( GL_DEPTH_TEST ); /* enable depth-testing */
    glDepthFunc( GL_LESS );

    vert_shader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vert_shader, 1, &vertex_shader, NULL );
    glCompileShader( vert_shader );
    frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( frag_shader, 1, &fragment_shader, NULL );
    glCompileShader( frag_shader );
    shader_programme = glCreateProgram();
    glAttachShader( shader_programme, frag_shader );
    glAttachShader( shader_programme, vert_shader );
    glLinkProgram( shader_programme );


    glfwSetCursorPosCallback(window,
            [](GLFWwindow *, double x, double y) {
            screen->cursorPosCallbackEvent(x, y);
        }
    );

    glfwSetMouseButtonCallback(window,
        [](GLFWwindow *, int button, int action, int modifiers) {
            screen->mouseButtonCallbackEvent(button, action, modifiers);
            glfw_mouse_click_callback(screen->glfwWindow(), button, action, modifiers);
        }
    );

    while ( !glfwWindowShouldClose( window ) ) {
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        if(!bezier_selected){
            if(points.size() > 0 && add_points){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                points[points.size() - 3] = x;
                points[points.size() - 2] = y;
            }

            if(points.size() > 0 && selected_point_index != -1){
                double xpos, ypos;
                glfwGetCursorPos( window, &xpos, &ypos );
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                double x = -1.0f + 2 * xpos/width;
                double y = +1.0f - 2 * ypos/height;
                points[selected_point_index - 3] = x;
                points[selected_point_index - 2] = y;
            }
            GLuint vbo = 0;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

            GLuint vao = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

            glUseProgram( shader_programme );
            glBindVertexArray( vao );

            glDrawArrays( GL_LINE_STRIP, 0, points.size()/3 );
            glPointSize(5);
            glDrawArrays( GL_POINTS, 0, points.size()/3 );
        }

        else{

            GLuint vbo = 0;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_points.size() * sizeof(GLfloat), draw_points.data(), GL_STATIC_DRAW);

            GLuint vao = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

            glUseProgram( shader_programme );
            glBindVertexArray( vao );

            glDrawArrays( GL_LINE_STRIP, 0, draw_points.size()/3 );

            vbo = 0;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

            vao = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

            glUseProgram( shader_programme );
            glBindVertexArray( vao );

            glPointSize(5);
            glDrawArrays( GL_POINTS, 0, points.size()/3 );


        }

        glfwPollEvents();
        screen->drawContents();
        screen->drawWidgets();

        glfwSwapBuffers( window );
    }

    glfwTerminate();
    return 0;
}
