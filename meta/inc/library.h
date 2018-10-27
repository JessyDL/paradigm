#pragma once
#include "stdafx_psl.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <memory>
#include "serialization.h"
#include "meta.h"

/// \brief contains utilities to identify types and instances at runtime and on disk.
///
/// small namespace that contains UID, meta::file, and meta::library
/// each of those are to be used together to identify types and instances on disk, or at runtime.
/// more detailed information can be found in the specific class pages.
namespace meta
{
	static const psl::string8_t META_EXTENSION = "meta";
	static const psl::string8_t LIBRARY_EXTENSION = META_EXTENSION + "lib";

	class library;

	/// \brief disk-based representation of a runtime instance.
	///
	/// This class contains associated helper data for a runtime instance, by default atleast a UID, polymorphic 
	/// information, and all associated tags are stored. 
	/// You can extend meta::file to add additional information that might be needed. See the "see also" section for examples.
	/// \see core::meta::shader 
	/// \see core::meta::texture
	class file
	{
		friend class serialization::accessor;
		friend class library;

	public:
		/// \param[in] key the unique UID associated with this meta::file
		file(const UID &key) : m_ID(key) {};

		template<typename S>
		file(S& s, const psl::string8_t& filename)  
		{
			static_assert(serialization::details::member_function_serialize<serialization::encode_to_format, file>::value, "this shouldn't run");
			s.template deserialize<serialization::decode_from_format>(*this, filename);
		};
		file() : m_ID(UID::invalid_uid) {};
		virtual ~file() = default;

		file(const file&) = delete;
		file(file&&) = delete;
		file& operator=(const file&) = delete;
		file& operator=(file&&) = delete;

		/// \returns the UID associated with this instance.
		const UID &ID() const { return m_ID.value; };

		/// \returns the serialization name.
		/// \see serialization for more information.
		psl::string8::view name() const { return "META"; };

		/// \returns all associated tags with this instance.
		const std::vector<psl::string8_t>& tags() const { return m_Tags.value; };

	protected:
		template<typename S>
		void serialize(S& serializer)
		{
			serializer << m_ID << m_Tags;
		}

	private:
		serialization::property<UID, const_str("UID", 3)> m_ID;
		serialization::property<std::vector<psl::string8_t>, const_str("TAGS", 4)> m_Tags;

		static constexpr const char serialization_name[5]{ "META" };
		static constexpr const char polymorphic_name[5]{"META"};
		virtual const uint64_t polymorphic_id() { return polymorphic_identity; }
		static const uint64_t polymorphic_identity;
	};

	/// \brief container class for meta::file's
	///
	/// a meta::library is a collection of meta::file instances. The library will do the basic bookkeeping, allowing you to search and retrieve
	/// meta::file instances based on their UID, or tags and narrow it down depending on the meta::file type as well.
	///
	/// There are also facilities to have UID's reference other UID's, and do bi-directional lookups for these relations. As well as the ability to
	/// load the persistent companion file that the meta::file describes (in case the UID satisfies is_physical_file() ), and to have that companion
	/// file cached for faster reloads.
	class library
	{
	public:
		/// \brief location on disk where the library can be found.
		///
		/// The constructor will try to load the given filepath, and then parse it. It will also create the minimal representation of each meta::file entry
		/// in the given file, with its tags, etc..
		/// \param[in] lib The filepath to which file should be loaded. This path can either be absolute or relative.
		library(psl::string8::view lib);
		~library();

		library(const library& other) = delete;
		library(library&& other) = default;
		library& operator=(const library& other) = delete;
		library& operator=(library&& other) = delete;

		/// \brief creates a new entry with a unique UID and a given type that is either, or derived of meta::file.
		template<typename MF = file>
		std::pair<const UID&, MF&> create()
		{
			return create<MF>(UID::generate());
		}

		/// \brief creates a new entry with the given UID and a given type that is either, or derived of meta::file.
		/// \param[in] uid The UID that should be associated with this entry.
		/// \warning giving an already present UID will result in an error log, and you will be given back the instance that is already present.
		template<typename MF = file>
		std::pair<const UID&, MF&> create(UID&& uid)
		{
			if (m_MetaData.find(uid) != m_MetaData.end())
			{
				LOG_ERROR ( "Tried to create a UID that already is present in the MetaLibrary! Returning the original");
				return std::pair<const UID&, MF&>(m_MetaData.find(uid)->first, *(static_cast<MF*>(m_MetaData.find(uid)->second.data.get())));
			}
			auto pair = m_MetaData.emplace(uid, std::make_unique<MF>(uid));
			pair.first->second.data.get()->m_ID = pair.first->first;
			return std::pair<const UID&, MF&>(pair.first->first, *(static_cast<MF*>(pair.first->second.data.get())));
		}

		/// \brief serializes the given UID to disk in case it is present, and an on-disk file.
		///
		/// When you send a UID to this method, the meta::library will verify that it has an entry with the given UID, and that it is linked
		/// to an on-disk location. When it satisfies those two requirements, it will then proceed to serialize the file to disk using the current
		/// values that are present.
		/// \param[in] uid The UID you wish to serialize.
		/// \returns true when the UID is both present in the library, and it is linked to an on-disk file.
		bool serialize(const UID& uid);

		/// \brief tries to remove the given UID from the library.
		///
		/// Will try to remove the given UID from the meta::library. On success it will notify all other meta::file's that it references, that they
		/// no longer are being referenced by this instance.
		/// 
		/// \a safe_mode controls wether you should avoid deleting this instance if others are still referencing this UID (true), or forcibly delete
		/// this instance regardless (false).
		/// \param[in] uid The UID you wish to remove.
		/// \param[in] safe_mode when true (default), will disallow removing as long as this item is being referenced by others.
		/// \returns true if the instance was removed or not.
		bool remove(const UID& uid, bool safe_mode = true);

		/// \brief searches the meta::library if it has an entry that corresponds to the given UID.
		/// \param[in] uid The UID to search for.
		/// \returns true if found.
		bool contains(const UID& uid) const;

		/// \brief searches for the first UID that is associated with the given tag.
		/// \param[in] tag The tag to look for, it is case-sensetive.
		/// \returns optionally the first found UID.
		std::optional<UID> find(psl::string8::view tag) const;

		/// \brief advanced version of find() that finds all instances of the given tag.
		/// \param[in] tag The tag to look for, it is case-sensetive.
		/// \returns an unordered_set of all UID's that have this tag.
		std::unordered_set<UID> find_all(psl::string8::view tag) const;

		/// \brief get all tags (if any) associated with the given UID.
		/// \param[in] uid the UID to get all tags for.
		/// \returns a list of all found tags.
		/// \note even if the UID is not present in the meta::library, this method will not fail, it will just find 0 tags.
		const std::vector<psl::string8_t>& tags(const UID& uid) const;

		/// \brief checks if the given UID has the specific tag associated to it.
		/// \param[in] uid the UID to check.
		/// \param[in] tag the associated tag.
		/// \returns true if the tag is present on the UID.
		/// \note if the UID is not present in the meta::library, then this method always returns false.
		bool has_tag(const UID& uid, psl::string8::view tag) const;

		/// \brief associates the tag with this specific UID.
		/// \param[in] uid the UID to associate with the tag.
		/// \param[in] tag the tag to apply to the UID.
		/// \returns if this was successful or not. It could return false in case the UID is not part of the meta::library.
		/// \warning it is possible to set the same tag multiple times on the same UID.
		bool set(const UID& uid, psl::string8::view tag);

		/// \brief advanced version of set() that allows a list of tags.
		/// \param[in] uid the UID to associate with the tags.
		/// \param[in] tags the tags to apply to the UID.
		/// \returns if this was successful or not. It could return false in case the UID is not part of the
		/// meta::library. 
		/// \warning it is possible to set the same tag multiple times on the same UID.
		bool set(const UID& uid, std::vector<psl::string8::view> tags);

		/// \brief gets a list of all UID's the given UID might be referencing.
		///
		/// as an example, a core::gfx::material could be referencing a core::gfx::texture, this means that
		/// if you call this method using the core::gfx::material's UID, then you will get a list back that
		/// atleast contains the core::gfx::texture UID.
		/// \param[in] uid the UID to find all references for.
		/// \returns a list of all UID's the given UID is referencing.
		/// \note will return an empty list in case the given UID is not present in the meta::library.
		std::unordered_set<UID> referencing(const UID& uid) const;

		/// \brief gets a list of all UID's the given UID might be referenced by.
		///
		/// as an example, a core::gfx::texture could be referenced by a core::gfx::material, this means that
		/// if you call this method using the core::gfx::texture's UID, then you will get a list back that
		/// atleast contains the core::gfx::material UID.
		/// \param[in] uid the UID for which to find all UID's that might be referencing this UID.
		/// \returns a list of all UID's the given UID is being referenced by.
		/// \note will return an empty list in case the given UID is not present in the meta::library.
		std::unordered_set<UID> referencedBy(const UID& uid) const;

		/// \brief loads the specific UID's content.
		///
		/// Loads the specified UID's associated file, **not** meta::file, and returns it (if found).
		/// The associated file is the companion file for which the meta::file was generated, for example a shader will have a core::meta::shader
		/// file on disk, but also the actual shader file (SPIR-V for Vulkan). This returns the SPIR-V file.
		/// The file content will get cached into the meta::library to make subsequent load() calls faster.
		/// \param[in] uid the given UID to optionally find the content for.
		/// \returns the resulting content in UTF-8 format if found.
		/// \warning this method requires the file to satisfy is_physical_file(), otherwise it will silently fail.
		std::optional<psl::string8::view> load(const UID& uid);

		/// \brief purges the specific UID's cachec content.
		///
		/// Unloads the specified UID's associated file, **not** meta::file, and returns true if it was in the cache and purged.
		/// The associated file is the companion file for which the meta::file was generated, for example a shader will have a core::meta::shader
		/// file on disk, but also the actual shader file (SPIR-V for Vulkan). This returns the SPIR-V file.
		/// You can reload the file into the cache by calling load() with the given UID as parameter.
		/// \param[in] uid the given UID to purge from the cache.
		/// \returns true if it was found on the meta::library, and had cached contents.
		/// \warning this method requires the file to satisfy is_physical_file(), otherwise it will silently fail.
		bool unload(const UID& uid);

		/// \brief get the UID's associated meta::file from the meta::library and tries to cast to the templated type.
		///
		/// Searches the meta::library for the given UID, and then tries to satisfy the templated type given. If successful
		/// it will return the given template type (default meta::file), otherwise it returns an std::nullopt in case the 
		/// conversion is not possible at runtime, and will give a compile error when the template type does not satisfy the type constraint.
		/// \tparam T should satisfy at least being either meta::file or being derived from meta::file
		/// \param[in] uid the given UID to find.
		/// \returns the meta::file, or derived class associated with the UID.
		/// \warning this method cannot downcast, it has to satisfy the type perfectly.
		template<typename T = file>
		std::optional<T*> get(const UID& uid) const;

		/// \brief checks if the given UID is associated with a physical/persistent file, or dynamically created during the runtime.
		/// \param[in] uid the UID to check.
		/// \returns true in case it has a persistent backing (that will survive a reboot).
		/// \note that the physical backing might be out of sync with the current version that is being used by the runtime.
		bool is_physical_file(const UID& uid) const;

		/// \brief gets the relative location (to the meta::library's location) of the given UID.
		/// \param[in] uid the UID to check.
		/// \returns the relative location to the meta::library if found.
		/// \note silently fails for UID's that are not present in the meta::library.
		std::optional<psl::string8_t> get_physical_location(const UID& uid) const;

		/// \brief returns the count of UID's present in the meta::library.
		/// \returns the amount of unique UID's in the current meta::library.
		size_t size() const;
	private:
		struct UIDData
		{
			UIDData(std::unique_ptr<file> &&dataPtr) : data(std::move(dataPtr)), flags(0) {};
			UIDData(file*&& dataPtr) : data(dataPtr), flags(0) {};
			UIDData() : data(nullptr), flags(0) {};

			std::unique_ptr<file> data;
			std::unordered_set<UID> referencing;
			std::unordered_set<UID> referencedBy;
			psl::string8_t file_data;

			// In case this is a file on disk, this defaults to the disk location, otherwise it's set from code.
			psl::string8_t readableName;

			std::bitset<3> flags;	/*	0: ON DISK FILE
									1: IS CACHE FILE
									2: IN CACHE FILE
									*/

		};

		std::unordered_map<psl::string8_t, std::unordered_set<UID>> m_TagMap;
		std::unordered_map<UID, UIDData> m_MetaData;

		psl::string8::view m_LibraryFile;
		psl::string8::view m_LibraryFolder;
		psl::string8_t m_LibraryLocation;
	};
	template<typename T>
	std::optional<T*> library::get(const UID & uid) const
	{
		if constexpr(std::is_same<T, ::meta::file>::value)
		{
			auto it = m_MetaData.find(uid);
			if(it == std::end(m_MetaData)) return {};

			return it->second.data.get();
		}
		else
		{
			static_assert(std::is_base_of<::meta::file, T>::value, "cannot cast the given type to meta::file, you can only use this to cast to derived types");


			auto it = m_MetaData.find(uid);
			if(it == std::end(m_MetaData)) return {};

			// todo: check the correctness of this
			if(it->second.data->serialization_name != serialization::accessor::name<T>())
				return {};

			T* res = reinterpret_cast<T*>(it->second.data.get());
			if(res != nullptr)
				return res;
			return {};
		}
	}
}
