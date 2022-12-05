# PSL
PSL contains both a method for disk based serialization of classes, as well as a data-format to serialize to and from.
In general the overhead for using the serialization is *none*, unless actual serialization happens. See psl::meta::file for an example of polymorphic serialization, and core::data::* classes for examples of static serialization.

## Static serialization
Static serialization is pretty trivial to use, and has no specific overhead. The compiler should be smart enough to inline most of the code that will be generated, and there are no indirections present in the system (aside from what you introduce) so it should all be transparent for the compiler to process and optimize.
```cpp
// example where we keep the serialization methods private
class static_serialization_example
{
    friend class serialization::accessor; // so we can access the needed information
                                          // this would be unneeded if void serialize() 
                                          // and serialization_name were public.
  public:
    /* your normal code here */
    
  private:
    // this will be invoked by the serializer.
    template<typename S>
    void serialize(S& serializer)
    {
        serializer << m_Name << m_Health;
    }
    // name that will be used if this is the root object, there should be no clashes
    // make sure this is unique for the scopes you will serialize to.
    static constexpr const char serialization_name[5]{"UNIT"};

    // next up are the properties we wish to serialize/deserialize
    serialization::property<int, const_str("HEALTH", 6)> m_Health;
    serialization::property<psl::string, const_str("NAME", 4)> m_Name;
}
```

## Polymorphic serialization
There's really only a couple of differences between polymorphic and static serialization, and that is that you *also* need to register your type for the polymorphic system, that will internally track an ID associated with your type, and create a lambda that wraps a generator for this type.
The extra overhead is creating a lookup table, and using the lookup table when deserializing.
### Base class
```cpp
// polymorphic_serialization_base_example.h
// example where we keep the serialization methods private
class polymorphic_serialization_base_example
{
    friend class serialization::accessor;
  public:
    /* your normal code here */
    
  private:
    // this will be invoked by the serializer.
    template<typename S>
    void serialize(S& serializer)
    {
        serializer << m_ID;
    }
    
    static constexpr const char serialization_name[10]{"UNIT_BASE"};
    
    /// the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64 ID
    /// of this type on.
    static constexpr const char polymorphic_name[5]{"META"};
    
    /// returns the polymorphic ID at runtime, to resolve what type this is.
    virtual const uint64_t polymorphic_id() { return polymorphic_identity; }
    
    /// the unique ID (per type, not instance) for the polymorphic system.
    static const uint64_t polymorphic_identity;
    
    serialization::property<int, const_str("ID", 2)> m_ID;
}
// eof

// polymorphic_serialization_base_example.cpp

// registers this in a translation unit for runtime usage.
const uint64_t polymorphic_serialization_base_example::polymorphic_identity
     {serialization::register_polymorphic<polymorphic_serialization_base_example>()};
// eof
```
### Derived class
```cpp
// derived_example.h
class derived_example : public polymorphic_serialization_base_example
{
    friend class serialization::accessor;
  public:
    /* your normal code here */
    
  private:
    template<typename S>
    void serialize(S& serializer)
    {
        // we call the base class (preferably) first.
        polymorphic_serialization_base_example::serialize(s);
        // then we serialize like normal.
        serializer << m_Armor;
    }

    /// the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64 ID
    /// of this type on.
    static constexpr const char polymorphic_name[10]{"TANK_UNIT"};
    /// returns the polymorphic ID at runtime, to resolve what type this is.
    virtual const uint64_t polymorphic_id() override { return polymorphic_identity; }
    /// the unique ID (per type, not instance) for the polymorphic system.
    static const uint64_t polymorphic_identity;

    // warning: no longer needed as the static version will use the base class!
    // static constexpr const char serialization_name[10]{"UNIT_BASE"};

    serialization::property<float, const_str("ARMOR", 5)> m_Armor;
}
// eof

// derived_example.cpp
const uint64_t derived_example::polymorphic_identity
    {serialization::register_polymorphic<derived_example>()};
// eof
```

# Extras
**knowing when the object is being serialized or deserialized**
If you wish to know, when `void serialize(S& serializer)` is being invoked, if the object will be serialized, or deserialized, you can achieve this with the compile time `serialization::details::is_encoder<S>::value`, which you can wrap in a `if constexpr` like so:
```cpp
template<typename S>
void serialize(S& serializer)
{
    if constexpr(serialization::details::is_encoder<S>::value)
    { // I'm being serialized.   }
    else
    { // I'm being deserialized. }
}
```

**dynamic properties**
If you don't want to use `serialization::property` in as your class member variable type, you can also dynamically create/destroy them during serialization.
```cpp
template<typename S>
void serialize(S& serializer)
{    
    serialization::property<int&, const_str("HEALTH", 6)> healthProp{ m_Health };
    serialization::property<psl::string&, const_str("NAME", 4)> nameProp{ m_Name };
    serializer << nameProp << healthProp;
}

psl::string m_Name;
int m_Health;
```
