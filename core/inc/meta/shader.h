#pragma once
#include "library.h"
#include "serialization.h"
#include "vk/stdafx.h"
#include "ustring.h"
#include <vector>
#include "gfx/types.h"

namespace core::meta
{
	/// \brief contains extensions for meta data when loading shader files.
	///
	/// shaders need various metadata that can describe the binding points and types of resources
	/// a SPIR-V might be expecting, as well as the pipeline stage it is assigned to.
	/// This extension to psl::meta::file contains that type of data.
	/// \todo add a way to deal with push_constants
	class shader final : public psl::meta::file
	{
		friend class psl::serialization::accessor;
		friend class library;

	  public:
		/// \brief contains data specific to vertex attributes.
		/// \note don't double subscribe attributes (i.e. binding 2 or more attributes to the same binding location)
		class vertex
		{
			vertex() = default;

		  public:
			/// \brief vertex attribute description.
			class attribute
			{
				friend class psl::serialization::accessor;

			  public:
				attribute();
				~attribute();
				attribute(const attribute&) = default;
				attribute(attribute&&)		= default;
				attribute& operator=(const attribute&) = default;
				attribute& operator=(attribute&&) = default;

				/// \brief returns the binding location of the attribute.
				/// \returns the binding location of the attribute.
				uint32_t location() const noexcept;

				/// \brief sets the binding location of the attribute.
				/// \param[in] value the location this attribute will bind to.
				void location(uint32_t value);

				/// \brief returns the format that this binding location should be using.
				/// \returns the format that this binding location should be using.
				///
				/// the format is what will be checked to see if you can bind the external resource
				/// to this binding spot.
				core::gfx::format format() const noexcept;
				/// \brief sets the format of this attribute.
				/// \param[in] value the format of this attribute.
				void format(core::gfx::format value);

				/// \brief returns the offset in bytes starting from the binding location.
				/// \returns the offset in bytes starting from the binding location.
				uint32_t offset() const noexcept;

				/// \brief sets the offset in bytes from the binding location.
				/// \param[in] value the offset in bytes from the binding location.
				void offset(uint32_t value);

			  private:
				/// \brief method that will be invoked by the serialization system.
				/// \tparam S the type of the serializer/deserializer
				/// \param[in] s instance of a serializer that you can read from, or write to.
				template <typename S>
				void serialize(S& s)
				{
					const uint32_t current_version = 1;
					psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
					s << version;

					switch(version)
					{
					case current_version: s << m_Location << m_Format << m_Offset; break;
					case 0:
						psl::serialization::property<vk::Format, const_str("FORMAT", 6)> format;
						s << format;
						m_Format.value = gfx::to_format(format.value);
						s << m_Location << m_Offset;
					}
				}
				/// \brief the serialization name for the psl::format::node
				static constexpr const char serialization_name[17]{"VERTEX_ATTRIBUTE"};
				psl::serialization::property<uint32_t, const_str("LOCATION", 8)> m_Location;
				psl::serialization::property<core::gfx::format, const_str("FORMAT", 6)> m_Format;
				psl::serialization::property<uint32_t, const_str("OFFSET", 6)> m_Offset;
			};

			/// \brief generic shader binding descriptor. describes the type and location of a vertex binding resource.
			///
			/// shaders can have various bindings such as textures, UBO's, SSBO's, Dynamic UBO's, etc...
			/// this class describes the type and either the psl::UID, or the tag that points to a resource in the
			/// runtime.
			class binding
			{
				friend class psl::serialization::accessor;

			  public:
				binding();
				~binding();
				binding(const binding&) = default;
				binding(binding&&)		= default;
				binding& operator=(const binding&) = default;
				binding& operator=(binding&&) = default;

				/// \returns the binding slot index of this binding
				uint32_t binding_slot() const noexcept;
				/// \returns the size in bytes of the binding
				uint32_t size() const noexcept;
				/// \returns the set input rate (per-vertex or per-instance) of this binding.
				core::gfx::vertex_input_rate input_rate() const noexcept;
				/// \returns the buffer this binding is bound to.
				/// \note this can either be a psl::UID, or a TAG
				psl::string8_t buffer() const noexcept;
				/// \returns the attribute collection of this binding.
				const std::vector<attribute>& attributes() const noexcept;

				/// \brief sets the binding slot to bind to in the shader.
				/// \param[in] value the new binding slot.
				void binding_slot(uint32_t value);

				/// \brief sets the binding size in bytes.
				/// \param[in] value the new accumulative size in bytes of this binding.
				void size(uint32_t value);
				/// \brief sets the expected input rate of the binding (i.e. should we offset per-vertex, or
				/// per-instance in the data) \param[in] value the new input rate.
				void input_rate(core::gfx::vertex_input_rate value);
				/// \brief sets the psl::UID, or TAG that will suggest where to find the data to bind.
				/// \details in core::data::geometry you will see some information about tags,
				/// these tags can be set here to identify the binding points of the geometry data in the shader
				/// i.e. what is the position binding, uv binding, etc..
				/// \see core::data::geometry core::ivk::geometry
				/// \param[in] value the new psl::UID or TAG
				void buffer(psl::string8_t value);
				/// \brief sets the collection of attributes that make up this binding.
				/// \param[in] value the attribute collection that will replace the current.
				void attributes(const std::vector<attribute>& value);

				/// \brief sets the attribute in case it is emplaced on an empty location.
				/// \param[in] value the attribute to try and emplace
				/// \deprecated prefer using the attributes(value) method
				[[deprecated("prefer using the attributes(value) method")]] void set(attribute value);
				/// \brief tries to find a matching attribute that is in the binding collection, and erases it if found
				/// \returns true if it found a match and erased it.
				/// \param[in] value the attribute to find.
				bool erase(attribute value);

			  private:
				/// \brief method that will be invoked by the serialization system.
				/// \tparam S the type of the serializer/deserializer
				/// \param[in] s instance of a serializer that you can read from, or write to.
				template <typename S>
				void serialize(S& s)
				{
					s << m_Binding << m_Size << m_InputRate << m_Buffer << m_Attributes;
				}
				/// \brief the serialization name for the psl::format::node
				static constexpr const char serialization_name[15]{"VERTEX_BINDING"};
				psl::serialization::property<uint32_t, const_str("BINDING", 7)> m_Binding;
				psl::serialization::property<uint32_t, const_str("SIZE", 4)> m_Size;
				psl::serialization::property<core::gfx::vertex_input_rate, const_str("INPUT_RATE", 10)> m_InputRate{
					core::gfx::vertex_input_rate::vertex};
				psl::serialization::property<psl::string8_t, const_str("BUFFER", 6)> m_Buffer;
				psl::serialization::property<std::vector<attribute>, const_str("ATTRIBUTES", 10)> m_Attributes;
			};
		};

		/// \brief describes the instance-able data contained in the SPIR-V module.
		///
		/// shaders can have many parameters that can be tweaked 'per-material instance' such as colors, or other
		/// values. This class describes those types of resources to make it easier and safer to dynamically bind data
		/// to instances of materials, and to give hints to the user of what can all be tweaked.
		/// A good example of "instance-able" data is object matrix data for instanced rendering. So that you can have
		/// many of the same objects rendered in "one call" through instanced rendering.
		/// you can view it as the finer detailed level core::meta::shader::descriptor
		class instance
		{
			friend class psl::serialization::accessor;

		  public:
			/// \brief Describes an element of a data container
			///
			/// Descriptors and instance data can consist of several smaller building blocks
			/// as those types can be viewed as "structs of data". But even something as simple
			/// as a matrix4x4 is actually a "struct of 4 times float[4]".
			/// This is what element describes, a collection of addressable elements (from the perspective of the GPU)
			/// of a struct of data.
			/// \warning elements are directly tied to their parent (either instance-, or descriptor binding).
			/// as their offset parameter is in relation to the parent.
			class element
			{
				friend class psl::serialization::accessor;

			  public:
				element();
				~element();
				element(const element&) = default;
				element(element&&)		= default;
				element& operator=(const element&) = default;
				element& operator=(element&&) = default;

				/// \returns the name of the element in the SPIR-V module (or the assigned one)
				///
				/// \details This is mostly for debugging purposes, it holds no other significance.
				psl::string8::view name() const noexcept;
				/// \returns the format that maps 1:1 to this element.
				core::gfx::format format() const noexcept;
				/// \returns the offset in bytes where this element lives in respect to the parent.
				uint32_t offset() const noexcept;
				/// \returns the default value (if any, otherwise defaulted) to fill in this element's data
				/// with if no override is given.
				const std::vector<uint8_t>& default_value() const noexcept;

				/// \brief sets the name of the element (mostly meant for user information)
				/// \param[in] value the new name to set.
				void name(psl::string8::view value);
				/// \brief format to bind this element to.
				/// \param[in] value the format that bests describes this element.
				void format(core::gfx::format value);
				/// \brief the offset from the parent in bytes.
				/// \param[in] value the offset from the parent structure start this element starts at.
				void offset(uint32_t value);

				/// \details default values are a handy way of emplacing data "by default"
				/// into a descriptor/instance. When binding fails, or when there is no "real"
				/// backing resource to bind to (could be in the case of setting the override color of a
				/// particle system), then the shader will be filled in with this value instead.
				/// Depending if it is a instance, or descriptor element, you can even set this value
				/// per-material.
				/// \param[in] value the default value if no override is given.
				void default_value(const std::vector<uint8_t>& value);

			  private:
				/// \brief method that will be invoked by the serialization system.
				/// \tparam S the type of the serializer/deserializer
				/// \param[in] s instance of a serializer that you can read from, or write to.
				template <typename S>
				void serialize(S& s)
				{
					const uint32_t current_version = 1;
					psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
					s << version;

					switch(version)
					{
					case current_version: s << m_Name << m_Format << m_Offset << m_Default; break;
					case 0:
						psl::serialization::property<vk::Format, const_str("FORMAT", 6)> format;
						s << format;
						m_Format.value = gfx::to_format(format.value);
						s << m_Name << m_Offset << m_Default;
					}
				}
				/// \brief the serialization name for the psl::format::node
				static constexpr const char serialization_name[24]{"SHADER_INSTANCE_ELEMENT"};
				psl::serialization::property<psl::string8_t, const_str("NAME", 4)> m_Name;
				psl::serialization::property<core::gfx::format, const_str("FORMAT", 6)> m_Format;
				psl::serialization::property<uint32_t, const_str("OFFSET", 6)> m_Offset;
				psl::serialization::property<std::vector<uint8_t>, const_str("DEFAULT", 7)> m_Default;
			};

			instance();
			~instance();
			instance(const instance&) = default;
			instance(instance&&)	  = default;
			instance& operator=(const instance&) = default;
			instance& operator=(instance&&) = default;

			/// \returns the collection of elements this isntance structure exists out of.
			const std::vector<element>& elements() const noexcept;
			/// \returns the accumulative size in bytes of elements().
			uint32_t size() const noexcept;
			/// \return a vector containing the "default" value state for this entire structure.
			std::vector<uint8_t> default_value() const noexcept;

			/// \brief overrides the elements that have been set with a new set.
			/// \param[in] value the new set of elements.
			void elements(const std::vector<element>& value);
			/// \brief sets a new size of this structure (in bytes)
			/// \param[in] value the new accumulative size in bytes of all elements.
			void size(uint32_t value);

			///	\brief tries to emplace this element in the structure definition, if the position is empty.
			/// \param[in] value the value to try emplace
			/// \deprecated prefer using the elements(value) method when possible
			[[deprecated("prefer using the elements(value) method when possible")]] void set(element value);
			/// \brief tries to find the given element in this structure and erase it.
			/// \returns true if found and erased.
			/// \param[in] value the element to find and erase.
			bool erase(element value);

			/// \brief tries to find the given element, by name, in this structure and erase it.
			/// \returns true if found and erased.
			/// \param[in] element_name the element's name that will be used to find and erase a matching element.
			bool erase(psl::string8::view element_name);

		  private:
			/// \brief method that will be invoked by the serialization system.
			/// \tparam S the type of the serializer/deserializer
			/// \param[in] s instance of a serializer that you can read from, or write to.
			template <typename S>
			void serialize(S& s)
			{
				s << m_Size << m_Elements;
			}
			/// \brief the serialization name for the psl::format::node
			static constexpr const char serialization_name[21]{"SHADER_INSTANCE_DATA"};
			psl::serialization::property<std::vector<element>, const_str("ELEMENTS", 8)> m_Elements;
			psl::serialization::property<uint32_t, const_str("SIZE", 4)> m_Size;
		};

		class descriptor
		{
			friend class psl::serialization::accessor;

		  public:
			descriptor();
			~descriptor();
			descriptor(const descriptor&) = default;
			descriptor(descriptor&&)	  = default;
			descriptor& operator=(const descriptor&) = default;
			descriptor& operator=(descriptor&&) = default;

			/// \returns true if the descriptor has a default value.
			bool has_default_value() const noexcept;

			/// \returns a vector containing the "default" value state for this structure.
			std::vector<uint8_t> default_value() const noexcept;

			/// \returns the binding slot for this resource.
			uint32_t binding() const noexcept;
			/// \returns the size in bytes of this resource.
			uint32_t size() const noexcept;
			/// \returns the name (optional) that can aid in giving the user information.
			psl::string8::view name() const noexcept;
			/// \returns the type of resource this descriptor uses in the shader module.
			core::gfx::binding_type type() const noexcept;
			/// \returns the set of elements that make up this descriptor resource.
			const std::vector<instance::element>& sub_elements() const noexcept;

			/// \brief sets a new binding slot for this resource.
			void binding(uint32_t value);

			/// \brief sets the new size in bytes of this resource
			/// \note the size can be 0 for special resources like samplers and textures.
			/// \param[in] value the size in bytes of all elements accumulated.
			void size(uint32_t value);
			/// \brief sets the name of this resource.
			/// \note not used for anything other than displaying to a user.
			/// \param[in] value the new name for this resource.
			void name(psl::string8::view value);
			/// \brief sets the type of resource the shader module expects.
			/// \param[in] value the new backing type that says what the shader module uses.
			/// \warning this needs to be accurate, or core::ivk::pipeline will fail to be bound to this shader module.
			void type(core::gfx::binding_type value);
			/// \brief sets the collection of sub-elements (1 to N) that make up this resource.
			/// \param[in] value the new set of instance elements of this descriptor
			void sub_elements(const std::vector<instance::element>& value);

			/// \brief tries to emplace, or update the instance element at the given binding.
			/// \note the binding location of the element sent to this method will be used to decide the match.
			/// \param[in] value the instance element to update, or emplace.
			/// \deprecated prefer using the sub_elements(value) method.
			[[deprecated("prefer using the sub_elements(value) method")]] void set(instance::element value);
			/// \brief tries to erase the instance element at the given binding.
			/// \note the binding location of the element sent to this method will be used to decide the match.
			/// \param[in] value the instance element to find and erase.
			/// \returns true on a successful erase.
			bool erase(instance::element value);

		  private:
			/// \brief method that will be invoked by the serialization system.
			/// \tparam S the type of the serializer/deserializer
			/// \param[in] s instance of a serializer that you can read from, or write to.
			template <typename S>
			void serialize(S& s)
			{
				s << m_Binding << m_Name << m_Size << m_SubElements;

				const uint32_t current_version = 1;
				psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
				s << version;

				switch(version)
				{
				case current_version: s << m_Type; break;
				case 0:
					psl::serialization::property<vk::DescriptorType, const_str("TYPE", 4)> type;
					s << type;
					m_Type.value = core::gfx::conversion::to_binding_type(type.value);
					break;
				}
			}

			/// \brief the serialization name for the psl::format::node
			static constexpr const char serialization_name[26]{"SHADER_BINDING_DESCRIPTOR"};

			psl::serialization::property<uint32_t, const_str("BINDING", 7)> m_Binding;
			psl::serialization::property<uint32_t, const_str("SIZE", 4)> m_Size;
			psl::serialization::property<psl::string8_t, const_str("NAME", 4)> m_Name;
			psl::serialization::property<core::gfx::binding_type, const_str("TYPE", 4)> m_Type;
			psl::serialization::property<std::vector<instance::element>, const_str("SUB_ELEMENTS", 12)> m_SubElements;
		};


		shader() = default;
		shader(const psl::UID& key) : psl::meta::file(key){};

		~shader() = default;

		/// \returns the shader stage of this SPIR-V module (i.e. vertex, fragment, compute, etc..)
		gfx::shader_stage stage() const noexcept;
		/// \brief sets the stage of this SPIR-V module to the given value.
		/// \warning it is assumed this stage flag is the actual stage flag, otherwise binding the
		/// shader will fail during creation.
		/// \param[in] value the stage to expect.
		void stage(gfx::shader_stage value) noexcept;

		/// \returns the collection of vertex bindings that this shader might have.
		const std::vector<vertex::binding>& vertex_bindings() const noexcept;
		/// \brief sets the expected set of vertex bindings of this SPIR-V module.
		/// \param[in] value the expected sets of vertex bindings.
		void vertex_bindings(const std::vector<vertex::binding>& value);

		/// \returns the set of descriptors that are used by this shader.
		const std::vector<descriptor>& descriptors() const noexcept;

		/// \brief sets the collection of descriptors that might be used by this shader.
		/// \param[in] value the new descriptors to expect bindings for.
		void descriptors(const std::vector<descriptor>& value);

		/// \brief tries to emplace, or overwrite (if sharing a binding location) a descriptor at the given binding
		/// slot.
		/// \param[in] value the descriptor to try to emplace.
		/// \deprecated prefer using descriptors(value) method
		[[deprecated("prefer using descriptors(value) method")]] void set(descriptor value);

		/// \brief tries to emplace, or overwrite (if sharing a binding location) a vertex binding at the given binding
		/// slot.
		/// \param[in] value the vertex binding to try and emplace.
		/// \deprecated prefer using vertex_bindings(value) method
		[[deprecated("prefer using vertex_bindings(value) method")]] void set(vertex::binding value);

		/// \brief tries to find, and erase a descriptor
		/// \note it uses the binding slot to verify which one to erase.
		/// \param[in] value the descriptor to find and erase.
		/// \returns true if the element has been found and erased.
		bool erase(descriptor value);
		/// \brief tries to find, and erase the vertex binding
		/// \note it uses the binding slot to verify which one to erase.
		/// \param[in] element the vertex binding to find and erase.
		/// \returns true if the element has been found and erased.
		bool erase(vertex::binding element);

		std::vector<vertex::binding> instance_bindings() const noexcept;
		std::optional<descriptor> material_data() const noexcept;

	  private:
		/// \brief method that will be invoked by the serialization system.
		/// \tparam S the type of the serializer/deserializer
		/// \param[in] s instance of a serializer that you can read from, or write to.
		template <typename S>
		void serialize(S& s)
		{
			psl::meta::file::serialize(s);
			const uint32_t current_version = 1;
			psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
			s << version;

			switch(version)
			{
			case current_version: s << m_Stage << m_VertexBindings << m_Descriptors; break;
			case 0:
				psl::serialization::property<vk::ShaderStageFlags, const_str("STAGE", 5)> stage;
				s << stage;
				m_Stage.value = gfx::to_shader_stage(stage.value);
				s << m_VertexBindings << m_Descriptors;
			}
		}


		psl::serialization::property<gfx::shader_stage, const_str("STAGE", 5)> m_Stage;
		psl::serialization::property<std::vector<vertex::binding>, const_str("VERTEX_BINDINGS", 15)> m_VertexBindings;
		psl::serialization::property<std::vector<descriptor>, const_str("DESCRIPTORS", 11)> m_Descriptors;

		/// \brief the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64
		/// ID of this type on.
		static constexpr const char polymorphic_name[12]{"SHADER_META"};
		/// \brief returns the polymorphic ID at runtime, to resolve what type this is.
		virtual const uint64_t polymorphic_id() override { return polymorphic_identity; }
		/// \brief the associated unique ID (per type, not instance) for the polymorphic system.
		static const uint64_t polymorphic_identity;
	};
} // namespace core::meta
