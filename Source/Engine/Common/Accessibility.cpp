#include "GraphicsPrimitive.h"

using namespace Inno;

Accessibility Accessibility::Immutable = Accessibility(false, false);
Accessibility Accessibility::ReadOnly = Accessibility(true, false);
Accessibility Accessibility::WriteOnly = Accessibility(false, true);
Accessibility Accessibility::ReadWrite = Accessibility(true, true);
Accessibility Accessibility::CopySource = Accessibility(true, false, true, false);
Accessibility Accessibility::CopyDestination = Accessibility(false, true, false, true);
