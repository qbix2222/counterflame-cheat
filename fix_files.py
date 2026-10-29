import re

# Fix cheat.h - add Metal import before typedefs
with open('cheat.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Add Metal import after the standard library includes
if '#include <random>' in content:
    content = content.replace(
        '#include <random>\n',
        '#include <random>\n#import <Metal/Metal.h>\n'
    )

# Remove duplicate extern declaration of g_currentRenderPassDescriptor (keep only one)
# The first declaration at line 223 is fine, but line 254 is a duplicate
# Remove the duplicate at line 254
lines = content.split('\n')
new_lines = []
skip_next = False
for i, line in enumerate(lines):
    if skip_next:
        skip_next = False
        continue
    # Remove duplicate g_currentRenderPassDescriptor extern (line 255 area)
    if 'extern id<MTLRenderPassDescriptor> g_currentRenderPassDescriptor;' in line:
        # Check if there's another one later - keep first, remove subsequent
        remaining = '\n'.join(lines[i+1:])
        if 'extern id<MTLRenderPassDescriptor> g_currentRenderPassDescriptor;' in remaining:
            skip_next = True
            continue
    new_lines.append(line)

content = '\n'.join(new_lines)

with open('cheat.h', 'w', encoding='utf-8') as f:
    f.write(content)

print('cheat.h fixed')

# Fix menu.mm - remove duplicate declarations that conflict with cheat.h externs
with open('menu.mm', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the duplicate declarations that conflict with extern declarations in cheat.h
# Lines that declare variables already declared extern in cheat.h:
# bool g_menu_visible = false;
static g_menu_visible_re = re.compile(r'^static bool g_menu_visible = false;$')
content = g_menu_visible_re.sub('', content)

float g_menu_slide_re = re.compile(r'^static float g_menu_slide   = 0\.0f;$')
content = g_menu_slide_re.sub('', content)

static float g_menu_target_re = re.compile(r'^static float g_menu_target  = 0\.0f;$')
content = g_menu_target_re.sub('', content)

static bool g_menu_toggle_btn_re = re.compile(r'^static bool  g_menu_toggle_btn = true;$')
content = g_menu_toggle_btn_re.sub('', content)

with open('menu.mm', 'w', encoding='utf-8') as f:
    f.write(content)

print('menu.mm fixed')
EOF
