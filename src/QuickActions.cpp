#include "QuickActions.h"

#include <I18n.h>

#include <vector>

#include "GlobalActions.h"
#include "components/OptionPopup.h"

namespace QuickActions {
void showConfiguredPopup(OptionPopup& popup, const std::function<void()>& requestUpdate) {
  std::vector<std::string> labels;
  std::vector<uint8_t> actions;
  labels.reserve(std::size(SETTINGS.quickActionSlots));
  actions.reserve(std::size(SETTINGS.quickActionSlots));
  for (const uint8_t action : SETTINGS.quickActionSlots) {
    if (action == CrossPointSettings::IGNORE || !isActionAvailable(action)) {
      continue;
    }
    labels.emplace_back(I18N.get(actionLabel(action)));
    actions.push_back(action);
  }
  if (actions.empty()) return;
  popup.show(StrId::STR_QUICK_ACTIONS, labels, 0, [actions = std::move(actions)](const int selected) {
    if (selected >= 0 && static_cast<size_t>(selected) < actions.size()) {
      dispatchShortcutAction(static_cast<CrossPointSettings::SHORT_PWRBTN>(actions[selected]));
    }
  });
  requestUpdate();
}
}  // namespace QuickActions
