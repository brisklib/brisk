#include <brisk/core/Text.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/window/OsDialogs.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

struct Person {
    std::string first, last;
    auto operator<=>(const Person&) const noexcept = default;
};

class Model {
public:
    const std::vector<Person>& list() const noexcept {
        return m_list;
    }

    size_t addPerson(const std::string& name, const std::string& surname) {
        m_list.push_back({ name, surname });
        return m_list.size() - 1;
    }

    void updatePerson(size_t index, const std::string& name, const std::string& surname) {
        if (index < m_list.size()) {
            m_list[index] = { name, surname };
        }
    }

    void deletePerson(size_t index) {
        if (index < m_list.size()) {
            m_list.erase(m_list.begin() + index);
        }
    }

private:
    std::vector<Person> m_list{ { "Hans", "Emil" }, { "Max", "Mustermann" }, { "Roman", "Tisch" } };
};

class ViewModel : public BindableObject<ViewModel, &uiScheduler> {
public:
    ViewModel() {
        filter(m_prefix);
        bindings->listen(Value{ &m_prefix }, BindableCallback<std::string>(this, &ViewModel::filter));
        bindings->listen(Value{ &m_selectedIndex }, BindableCallback<int>(this, &ViewModel::read));
    }

    void read(int index) {
        if (index >= 0) {
            *bindings->modify(m_name)    = m_filteredList[index].first.first;
            *bindings->modify(m_surname) = m_filteredList[index].first.last;
        } else {
            *bindings->modify(m_name)    = {};
            *bindings->modify(m_surname) = {};
        }
    }

    void filter(const std::string& filter) {
        m_filteredList.clear();
        for (size_t i = 0; i < m_model.list().size(); ++i) {
            if (matches(m_model.list()[i], filter)) {
                m_filteredList.push_back({ m_model.list()[i], i });
            }
        }
        bindings->notify(&m_filteredList);
    }

    void create() {
        size_t newIndex = m_model.addPerson(m_name, m_surname);
        selectedIndex   = -1;
        filter(m_prefix);
    }

    void update() {
        size_t unfilteredIndex = m_filteredList.at(m_selectedIndex).second;
        m_model.updatePerson(unfilteredIndex, m_name, m_surname);
        filter(m_prefix);
    }

    void delete_() {
        size_t unfilteredIndex = m_filteredList.at(m_selectedIndex).second;
        m_model.deletePerson(unfilteredIndex);
        selectedIndex = -1;
        filter(m_prefix);
    }

private:
    static bool matches(const Person& person, std::string_view prefix) {
        auto prefixLower = lowerCase(prefix);
        return lowerCase(person.first).starts_with(prefixLower) ||
               lowerCase(person.last).starts_with(prefixLower);
    }

    Model m_model;
    std::vector<std::pair<Person, size_t>> m_filteredList;
    std::string m_prefix, m_name, m_surname;
    int m_selectedIndex = -1;

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &ViewModel::m_filteredList },
            /*1*/ Internal::PropField{ &ViewModel::m_prefix },
            /*2*/ Internal::PropField{ &ViewModel::m_name },
            /*3*/ Internal::PropField{ &ViewModel::m_surname },
            /*4*/ Internal::PropField{ &ViewModel::m_selectedIndex },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ViewModel, std::vector<std::pair<Person, size_t>>, 0> filteredList;
    Property<ViewModel, std::string, 1> prefix;
    Property<ViewModel, std::string, 2> name;
    Property<ViewModel, std::string, 3> surname;
    Property<ViewModel, int, 4> selectedIndex;
    BRISK_PROPERTIES_END
};

class View final : public Component {
private:
    Rc<ViewModel> m_viewModel;

public:
    View(Rc<ViewModel> viewModel) : m_viewModel(std::move(viewModel)) {}

    Rc<Widget> build() final {
        return rcnew VLayout{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),

            minWidth = 250_px,
            padding  = 1_em,
            gap      = 0.5_em,
            rcnew HLayout{
                gap      = 0.5_em,
                flexGrow = 1,
                rcnew VLayout{
                    flexGrow  = 1,
                    gap       = 0.5_em,
                    flexBasis = 0,
                    rcnew HLayout{
                        "Filter prefix:"_Text,
                        gap = 0.5_em,
                        rcnew TextEditor{ text = Value{ &m_viewModel->prefix }, flexGrow = 1 },
                    },
                    rcnew ListBox{
                        flexGrow = 1,
                        value    = Value{ &m_viewModel->selectedIndex },
                        depends  = transform(
                            [](const auto&, const auto&) -> Trigger<> {
                                return {};
                            },
                            Value{ &m_viewModel->filteredList }, Value{ &m_viewModel->prefix }),
                        Builder{
                            [this](Widget* list) {
                                for (auto name : m_viewModel->filteredList.get()) {
                                    list->apply(rcnew Item{
                                        rcnew Text{ fmt::format("{} {}", name.first.first, name.first.last) },
                                    });
                                }
                            },
                        },
                    },
                },
                rcnew VLayout{
                    flexGrow  = 1,
                    gap       = 0.5_em,
                    flexBasis = 0,
                    rcnew HLayout{
                        rcnew Text{ "Name:", &m_sameWidth },
                        gap = 0.5_em,
                        rcnew TextEditor{ text = Value{ &m_viewModel->name }, flexGrow = 1 },
                    },
                    rcnew HLayout{
                        rcnew Text{ "Surname:", &m_sameWidth },
                        gap = 0.5_em,
                        rcnew TextEditor{ text = Value{ &m_viewModel->surname }, flexGrow = 1 },
                    },
                },
            },
            rcnew HLayout{
                gap = 0.5_em,
                rcnew Button{
                    "Create"_Text,
                    onClick = BindableCallback<>{ m_viewModel.get(), &ViewModel::create },
                },
                rcnew Button{
                    "Update"_Text,
                    onClick = BindableCallback<>{ m_viewModel.get(), &ViewModel::update },
                    enabled = Value{ &m_viewModel->selectedIndex } >= 0,
                },
                rcnew Button{
                    "Delete"_Text,
                    onClick = BindableCallback<>{ m_viewModel.get(), &ViewModel::delete_ },
                    enabled = Value{ &m_viewModel->selectedIndex } >= 0,
                },
            },
        };
    }

    void configureWindow(Rc<GuiWindow> window) final {
        window->setTitle("Timer"_tr);
    }

    WidthGroup m_sameWidth;
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GuiApplication application;
    return application.run([]() {
        return rcnew View(rcnew ViewModel());
    });
}
