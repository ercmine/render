$input a_position, a_color0
$output v_color0

#include "../includes/core/common.sh"
#include "../includes/core/transform.sh"

void main()
{
    gl_Position = render_transform_position(a_position);
    v_color0 = a_color0;
}
