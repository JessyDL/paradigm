#pragma once
#include "resource/resource.hpp"
#include "psl/unique_ptr.h"
#include "psl/view_ptr.h"
#include "psl/array_view.h"
#include <variant>

namespace core::gfx
{
	class context;
	class framebuffer;
	class swapchain;
	class drawpass;
	class computepass;
} // namespace core::gfx


namespace psl
{

	template <typename T>
	class graph
	{
		struct node
		{
			T value;
			size_t references{0};
		};

	  public:

		  ~graph()
		  {
			  for (auto& [node, list] : m_Edges)
			  {
				  delete(node);
			  }
		  }
		template <typename... Args>
		T* emplace(Args&&... args)
		{
			m_Edges.emplace_back(std::pair<node*, psl::array<node*>>{new node{T{std::forward<Args>(args)...}}, {}});
			m_Heads.emplace_back(&m_Edges.back().first->value);
			return &m_Edges.back().first->value;
		}

		template <typename Key, typename Fn>
		T* find_if(const Key& value,Fn&& fn ) const noexcept
		{
			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&value, &fn](const auto& pair) { return fn(pair.first->value, value); });
			if (it == std::end(m_Edges)) return nullptr;
			return &(it->first->value);
		}

		template <typename Key>
		T* find(const Key& value) const noexcept
		{
			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
								   [&value](const auto& pair) { return pair.first->value == value; });
			if(it == std::end(m_Edges)) return nullptr;
			return &(it->first->value);
		}

		bool connect(T* source, T* target)
		{
			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
								   [&source](const auto& pair) { return &pair.first->value == source; });
			if(it == std::end(m_Edges)) return false;

			auto target_it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&target](const auto& pair) { return &pair.first->value == target; });
			if (target_it == std::end(m_Edges)) return false;

			{ // we erase the target from the heads list in case it was present.
				auto target_head_it = std::find(std::begin(m_Heads), std::end(m_Heads), target);
				if(target_head_it != std::end(m_Heads))
				{
					m_Heads.erase(target_head_it);
				}
			}

			++(target_it->first->references);
			it->second.emplace_back(target_it->first);
			return true;
		}

		bool disconnect(T* source, T* target) 
		{
			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&source](const auto& pair) { return &pair.first->value == source; });
			if (it == std::end(m_Edges)) return false;

			auto target_it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&target](const auto& pair) { return &pair.first->value == target; });
			if (target_it == std::end(m_Edges)) return false;

			auto found_it = std::find(std::begin(it->second), std::end(it->second), target_it->first);
			if (found_it == std::end(it->second)) return false;


			--(target_it->first->references);
			if (target_it->first->references == 0)
			{
				m_Heads.emplace_back(&(target_it->first->value));
			}

			it->second.erase(found_it);
			return true; 
		}

		bool disconnect(T* source)
		{
			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&source](const auto& pair) { return &pair.first->value == source; });
			if (it == std::end(m_Edges)) return false;

			if (it->first->references > 0)
			{
				for (auto& [node, list] : m_Edges)
				{
					disconnect(&node->value, &it->first->value);
				}
			}

			assert(it->first->references == 0);

			for (auto* node : it->second)
			{
				--node->references;
				if (node->references == 0)
				{
					m_Heads.emplace_back(&node->value);
				}
			}

			m_Heads.emplace_back(&it->first->value);
			return true;
		}

		bool erase(T* source)
		{
			disconnect(source);

			auto it = std::find_if(std::begin(m_Edges), std::end(m_Edges),
				[&source](const auto& pair) { return &pair.first->value == source; });
			if (it == std::end(m_Edges)) return false;

			delete(it->first);
			m_Edges.erase(it);
			m_Heads.erase(std::find(std::begin(m_Heads), std::end(m_Heads), source));
			return true;
		}

		template <typename It>
		bool is_head(T* value) const noexcept
		{
			return std::find(std::begin(m_Heads), std::end(m_Heads), value) != std::end(m_Heads);
		}

		psl::array<T*> to_array() const noexcept
		{
			// todo: bruteforce algorithm, this is a prime location for improvement

			psl::array<T*> result{ m_Heads };
			
			std::unordered_map<const T*, bool> visited{};
			for (const auto& [node, list] : m_Edges)
				visited[&node->value] = false;
			for (auto node : m_Heads)
				visited[node] = true;

			while (result.size() != m_Edges.size())
			{
				for (const auto& [node, list] : m_Edges)
				{
					if (node->references == 0 || visited[&node->value])
					{
						for (auto node_ref : list)
						{
							if ((node_ref->references == 1 || std::find(std::begin(result), std::end(result), &node_ref->value) == std::end(result)) && !visited[&node_ref->value])
							{
								visited[&node_ref->value] = true;
								result.emplace_back(&node_ref->value);
							}
						}
					}
				}
			}

			return result;
		}

	  private:
		psl::array<T*> m_Heads;
		psl::array<std::pair<node*, psl::array<node*>>> m_Edges;
	};
} // namespace psl
namespace core::gfx
{
	class render_graph
	{
	  public:
		using view_var_t = std::variant<psl::view_ptr<core::gfx::drawpass>, psl::view_ptr<core::gfx::computepass>>;
		using unique_var_t =
			std::variant<psl::unique_ptr<core::gfx::drawpass>, psl::unique_ptr<core::gfx::computepass>>;

	  public:
		~render_graph();

		psl::view_ptr<core::gfx::drawpass> create_drawpass(core::resource::handle<core::gfx::context> context,
														   core::resource::handle<core::gfx::swapchain> swapchain);
		psl::view_ptr<core::gfx::drawpass> create_drawpass(core::resource::handle<core::gfx::context> context,
														   core::resource::handle<core::gfx::framebuffer> framebuffer);
		psl::view_ptr<core::gfx::computepass>
		create_computepass(core::resource::handle<core::gfx::context> context) noexcept;
		bool connect(view_var_t child, view_var_t root) noexcept;
		bool disconnect(view_var_t pass) noexcept;
		bool disconnect(view_var_t child, view_var_t root) noexcept;

		bool erase(view_var_t pass) noexcept;
		void present();

	  private:
		void rebuild() noexcept;

		psl::graph<unique_var_t> m_RenderGraph;
		psl::array<unique_var_t*> m_FlattenedRenderGraph;

		bool m_Rebuild{true};
	};
} // namespace core::gfx