#include "pch.h"
#include "Toad/toadll.h"
#include "mappings.h"
#include "mapping_generator.h"
#include "mcutils.h"
#include "nlohmann/json.hpp"

#include "../../Loader/src/Application/config.h"

#define JVMTICALL(f, ...)													\
if (auto res = f(__VA_ARGS__); res != jvmtiError::JVMTI_ERROR_NONE)			\
{																			\
	LOGERROR("[MappingGenerator] Failed to call {}, {}", #f, (int)res);		\
	return;																	\
}

namespace toadll
{

using json = nlohmann::json;

json Mappings::Serialize()
{
	json data;
	json methods_data;
	json fields_data;

	for (size_t i = 0; i < fields.size(); i++)
	{
		json field_data;

		field_data["field"] = fields[i].field;
		field_data["modifiers"] = fields[i].modifiers;
		field_data["signature"] = fields[i].signature;
		field_data["name"] = fields[i].name;
		field_data["index"] = fields[i].index;

		fields_data[i] = field_data;
	}

	for (size_t i = 0; i < methods.size(); i++)
	{
		json method_data;
		method_data["method"] = methods[i].method;
		method_data["modifiers"] = methods[i].modifiers;
		method_data["signature"] = methods[i].signature;
		method_data["name"] = methods[i].name;
		method_data["bytecodes"] = methods[i].bytecodes;
		method_data["index"] = methods[i].index;

		methods_data[i] = method_data;
	}

	data["methods"] = methods_data;
	data["fields"] = fields_data;
	return data;
}

Mappings Mappings::Deserialize(const json& data)
{
	Mappings mappings;
	json methods_data;
	json fields_data;

	config::get_json_element(methods_data, data, "methods");
	config::get_json_element(fields_data, data, "fields");

	for (size_t i = 0; i < methods_data.size(); i++)
	{
		MethodMapping method;
		json method_data = methods_data[i];
		config::get_json_element(method.method, method_data, "method");
		config::get_json_element(method.name, method_data, "name");
		config::get_json_element(method.signature, method_data, "signature");
		config::get_json_element(method.modifiers, method_data, "modifiers");
		config::get_json_element(method.bytecodes, method_data, "bytecodes");
		config::get_json_element(method.index, method_data, "index");

		mappings.methods.emplace_back(method);
	}

	for (size_t i = 0; i < fields_data.size(); i++)
	{
		MappingField field;
		json field_data = fields_data[i];
		config::get_json_element(field.field, field_data, "field");
		config::get_json_element(field.name, field_data, "name");
		config::get_json_element(field.signature, field_data, "signature");
		config::get_json_element(field.modifiers, field_data, "modifiers");
		config::get_json_element(field.index, field_data, "index");
		mappings.fields.emplace_back(field);
	}
	
	return mappings;
}

void MappingGenerator::Generate(JNIEnv* env, jvmtiEnv* jvmti_env)
{
	CHAR documents[MAX_PATH];
	HRESULT get_folder_path_res = SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents);
	if (get_folder_path_res != S_OK)
	{
		LOGERROR("[MappingGenerator] Failed to get documents folder");
		return;
	}

	const auto serialize_klass = [&env, &jvmti_env](json& data, std::string_view klass_name, std::string_view json_name)
		{
			// get info 
			jclass klass = findclass(klass_name.data(), env);
			int index = -1;
			Mappings mappings = GetMappingsForClass(env, jvmti_env, klass, index);
			env->DeleteLocalRef(klass);

			// store in json 
			data[json_name]["mappings"] = mappings.Serialize();
			data[json_name]["class_index"] = index;
		};

	json data;
	serialize_klass(data, "net.minecraft.client.Minecraft", "mc");
	serialize_klass(data, "net.minecraft.client.entity.EntityPlayerSP", "player");
	serialize_klass(data, "net.minecraft.util.MovingObjectPosition", "mop");
	serialize_klass(data, "net.minecraft.world.World", "world");
	serialize_klass(data, "net.minecraft.entity.EntityLivingBase", "elb");
	serialize_klass(data, "net.minecraft.entity.Entity", "entity");
	serialize_klass(data, "net.minecraft.client.renderer.ActiveRenderInfo", "ari");
	serialize_klass(data, "net.minecraft.util.Vec3i", "vec3i");
	serialize_klass(data, "net.minecraft.util.Vec3", "vec3");
	serialize_klass(data, "net.minecraft.util.BlockPos", "blockpos");
	serialize_klass(data, "net.minecraft.entity.player.EntityPlayer", "entityplayer");
	serialize_klass(data, "net.minecraft.util.Timer", "timer");
	serialize_klass(data, "net.minecraft.client.gui.inventory.GuiChest", "guichest");
	serialize_klass(data, "net.minecraft.item.ItemStack", "itemstack");
	serialize_klass(data, "net.minecraft.block.Block", "block");
	serialize_klass(data, "net.minecraft.block.state.BlockState", "blockstate");
	serialize_klass(data, "net.minecraft.inventory.IInventory", "iinventory");

	std::ofstream file(std::filesystem::path(documents) / "mapping_gen_out.txt");
	if (!file)
	{
		LOGERROR("[MappingGenerator] Failed to create output file");
		return;
	}
	file << data << std::endl;
	file.close();
}

//void MappingGenerator::GetMappingsFromFile(JNIEnv* env, jvmtiEnv* jvmti_env, const std::filesystem::path& json_file)
//{
//	std::ifstream file(json_file);
//	if (!file)
//	{
//		LOGERROR("[MappingGenerator] Can't open: {}", json_file.string());
//		return;
//	}
//	std::stringstream ss;
//	ss << file.rdbuf();
//	file.close();
//
//	json data;
//	try
//	{
//		data = json::parse(ss.str());
//	}
//	catch (json::parse_error& e)
//	{
//		LOGERROR("[MappingGenerator] json parse error: {}", e.what());
//		return;
//	}
//
//	json mc_data;
//	json mappings_data;
//	if (!config::get_json_element(mc_data, data, "mc"))
//	{
//		LOGERROR("[MappingGenerator] No mc found in file: {}", json_file.string());
//		return;
//	}
//	if (!config::get_json_element(mappings_data, mc_data, "mappings"))
//	{
//		LOGERROR("[MappingGenerator] No mappings found in file: {}", json_file.string());
//		return;
//	}
//
//	Mappings mc_mappings = Mappings::Deserialize(mappings_data);
//
//	jclass mc = Minecraft::getMcClass(env);
//	std::string mc_class_name; 
//	if (!mc)
//	{
//		mc_class_name = FindClassTypes(env, jvmti_env, mc_mappings);
//
//		mc = findclass(mc_class_name.c_str(), env);
//
//		if (!mc)
//		{
//			LOGERROR("[MappingGenerator] Minecraft class can't be found");
//			return;
//		}
//	}
//
//	jint field_count = 0;
//	jfieldID* fields;
//
//	jvmtiError res;
//	res = jvmti_env->GetClassFields(mc, &field_count, &fields);
//	if (res != jvmtiError::JVMTI_ERROR_NONE)
//	{
//		LOGERROR("[MappingGenerator] Failed to call GetClassFields, {}", (int)res);
//		env->DeleteLocalRef(mc);
//		return;
//	}
//
//	//std::unordered_map<mapping, MCMap> methods;
//	std::unordered_map<mappingFields, MCMap> mc_fields{};
//
//	for (int i = 0; i < field_count; i++)
//	{
//		char* name;
//		char* sig;
//		char* gen;
//		if (jvmti_env->GetFieldName(mc, fields[i], &name, &sig, &gen) != JVMTI_ERROR_NONE)
//			continue;
//
//		// theMinecraft
//		if (strncmp(name, ('L' + mc_class_name + ';').c_str(), mc_class_name.size()) == 0)
//		{
//			mc_fields[mappingFields::theMcField] = MCMap{ name, sig };
//		}
//	}
//}

void MappingGenerator::InitMappings(JNIEnv* env, jvmtiEnv* jvmti_env, const std::filesystem::path& file)
{
	std::ifstream f(file);
	if (!f)
		return;

	json data; 
	try
	{
		data = json::parse(f);
	}
	catch (json::parse_error& e)
	{
		return;
	}

	const auto init_for_data = [&env, &jvmti_env, &data](std::string_view json_key)
		{
			json klass_data;
			config::get_json_element(klass_data, data, json_key);
			Mappings klass_mappings = Mappings::Deserialize(klass_data["mappings"]);
			std::string klass_name = FindClassTypes(env, jvmti_env, klass_mappings);
			jclass klass = findclass(klass_name.c_str(), env);
			InitMappingsForClass(env, jvmti_env, klass, klass_mappings);
			env->DeleteLocalRef(klass);
		};

	//FindClassTypes(env, jvmti_env, klass_mappings);
	//InitMappingsForClass(env, jvmti_env, klasses, klass_mappings);
	// #TODO: optimize later 

	// #TODO: sorted methods count 
	//std::queue<int> min_methods_count;
	//min_methods_count.push(0);

	json mc_data;
	config::get_json_element(mc_data, data, "mc");
	Mappings mc_mappings = Mappings::Deserialize(mc_data["mappings"]);
	std::string mc_class_name = FindClassTypes(env, jvmti_env, mc_mappings);
	jclass minecraft = findclass(mc_class_name.c_str(), env);
	InitMappingsForClass(env, jvmti_env, minecraft, mc_mappings);
	Minecraft::unsupported_mc_class_name = mc_class_name;
	env->DeleteLocalRef(minecraft);

	//init_for_data("player");
	//init_for_data("mop");
	//init_for_data("world");
	//init_for_data("elb");
	//init_for_data("entity");
	//init_for_data("ari");
	//init_for_data("vec3i");
	//init_for_data("vec3");
	//init_for_data("blockpos");
	//init_for_data("entityplayer");
	//init_for_data("timer");
	//init_for_data("guichest");
	//init_for_data("itemstack");
	//init_for_data("block");
	//init_for_data("blockstate");
	//init_for_data("iinventory");

	std::set<int> mappings_methods_initialized{};
	std::set<int> mappings_fields_initialized{};

	for (const auto& [mapping, map] : mappings::methods)
	{
		LOGDEBUG("{} ({}, {})", (int)mapping, map.name, map.sig);
		mappings_methods_initialized.emplace((int)mapping);
	}	
	
	for (const auto& [mapping, map] : mappings::fields)
	{
		LOGDEBUG("{} ({}, {})", (int)mapping, map.name, map.sig);
		mappings_fields_initialized.emplace((int)mapping);
	}

	// print missing 
	LOGDEBUG("[MappingGenerator] Missing {} methods", (int)mapping::COUNT - mappings_methods_initialized.size() - 1);
	LOGDEBUG("[MappingGenerator] Missing {} fields", (int)mappingFields::COUNT - mappings_fields_initialized.size() - 1);
	for (int i = 1; i < (int)mappingFields::COUNT - 1; i++)
	{
		if (mappings_fields_initialized.contains(i))
			continue;

		LOGDEBUG("[MappingGenerator] Missing field for {}", i);
	}	
	for (int i = 1; i < (int)mapping::COUNT - 1; i++)
	{
		if (mappings_methods_initialized.contains(i))
			continue;

		LOGDEBUG("[MappingGenerator] Missing method for {}", i);
	}
}

std::string MappingGenerator::FindClassTypes(JNIEnv* env, jvmtiEnv* jvmti_env, const Mappings& mappings)
{
	// to string method id for class names
	jclass klass = findclass("java/lang/Class", g_env);
	if (!klass)
	{
		LOGERROR("[MappingGenerator] Can't find class Class");
		return "";
	}
	jmethodID get_klass_name = g_env->GetMethodID(klass, "getName", "()Ljava/lang/String;");
	if (!get_klass_name)
	{
		LOGERROR("[MappingGenerator] Can't find String getName()");
		env->DeleteLocalRef(klass);
		return "";
	}
	env->DeleteLocalRef(klass);

	std::vector<std::vector<uint8_t>> mapping_method_bytecodes;
	mapping_method_bytecodes.reserve(mappings.methods.size());
	for (MethodMapping method : mappings.methods)
	{
		method.bytecodes.emplace_back(method.modifiers);
		method.bytecodes.emplace_back(method.index);
		mapping_method_bytecodes.emplace_back(method.bytecodes);
	}

	LOGDEBUG("[MappingGenerator] Mapping bytecodes size: {}", mapping_method_bytecodes.size());

	jint class_count = 0;
	jclass* classes;
	jvmti_env->GetLoadedClasses(&class_count, &classes);

	LOGDEBUG("[MappingGenerator] Classes: {}", class_count);

	std::vector<std::pair<std::string, float>> possible_classes{};

	for (int i = 0; i < class_count; i++)
	{
		// get methods
		jint methods_count = 0;
		jmethodID* methods = nullptr;
		jvmtiError err = jvmti_env->GetClassMethods(classes[i], &methods_count, &methods);

		float methods_count_diff = abs(methods_count - (int)mappings.methods.size());
		if (err != JVMTI_ERROR_NONE || 
			methods_count == 0 ||
			mappings.methods.size() == 0 || 
			methods_count_diff / mappings.methods.size() < 0.65f)
		{
			if (methods)
				jvmti_env->Deallocate((unsigned char*)methods);
			continue;
		}

		float similarity_score = 0;
		int similar_counter = 0;

		for (int j = 0; j < methods_count; j++)
		{
			uint8_t* bytecodes;
			jint bytecode_count = 0;

			// returns error 104 JVMTI_ERROR_NATIVE_METHOD sometimes
			err = jvmti_env->GetBytecodes(methods[j], &bytecode_count, &bytecodes);

			if (err != JVMTI_ERROR_NONE)
				continue;
			if (bytecode_count == 0)
				continue;

			std::vector<uint8_t> current_bytecodes(bytecodes, bytecodes + bytecode_count);
			std::set<int> ignore_method_bytecodes{};

			jint mod = -1;
			jvmti_env->GetMethodModifiers(methods[j], &mod);
			current_bytecodes.emplace_back(mod);
			current_bytecodes.emplace_back(j);

			// file mappings
			for (int k = 0; k < mapping_method_bytecodes.size(); k++)
			{
				if (ignore_method_bytecodes.contains(k))
					continue;
				if (mapping_method_bytecodes[k].empty() || current_bytecodes.empty())
					continue;

				float similarity = math::jaccard_index(current_bytecodes, mapping_method_bytecodes[k]);
				if (similarity > 0.5f)
				{
					similar_counter++;
					similarity_score += similarity;
					ignore_method_bytecodes.emplace(k);
				}
			}

			jvmti_env->Deallocate(bytecodes);
		}

		if (methods)
			jvmti_env->Deallocate((unsigned char*)methods);

		//LOGDEBUG("[MappingGenerator] {} / {} = {}", similar_counter, methods_count, (float)similar_counter / ((float)methods_count + FLT_EPSILON));

		if ((float)similar_counter / ((float)methods_count + FLT_EPSILON) > 0.5f)
		{
			LOGDEBUG("[MappingGenerator] Found a possibility with score {}", similarity_score);
			jstring klass_name = (jstring)env->CallObjectMethod(classes[i], get_klass_name);

			if (!klass_name)
			{
				LOGERROR("[MappingGenerator] Can't get name of possible minecraft class");
				break;
			}

			// add to possible minecraft class 
			possible_classes.emplace_back(jstring2string(klass_name, env), similarity_score);

			env->DeleteLocalRef(klass_name);
		}
	}

	jvmti_env->Deallocate((unsigned char*)classes);

	LOGDEBUG("[MappingGenerator] Class possibilities: {}", possible_classes.size());
	float max_score = 0;
	int index = -1;
	for (int i = 0; i < possible_classes.size(); i++)
	{
		const auto& [name, score] = possible_classes[i];
		if (score > max_score)
		{
			index = i;
			max_score = score;
		}
	}

	if (index != -1)
	{
		LOGDEBUG("[MappingGenerator] Found at {} with score {} name: {}", index, max_score, possible_classes[index].first);
		return possible_classes[index].first;
	}
	else
	{
		LOGDEBUG("[MappingGenerator] Couldn't find class");
		return "";
	}
}

float get_similarity(jclass klass, const Mappings& mapping)
{
	return 0;
}

Mappings MappingGenerator::GetMappingsForClass(JNIEnv* env, jvmtiEnv* jvmti_env, jclass klass, int& class_index)
{
	if (!klass)
	{
		LOGERROR("[MappingGenerator] klass argument is null");
		return {};
	}
	Mappings klass_mappings;
	jint field_count = 0;
	jfieldID* fields;

	jvmtiError jvmti_err;
	//JVMTICALL(jvmti_env->GetClassFields, mc, &field_count, &fields)
	jvmti_err = jvmti_env->GetClassFields(klass, &field_count, &fields);
	if (jvmti_err != jvmtiError::JVMTI_ERROR_NONE)
	{
		LOGERROR("[MappingGenerator] Failed to call GetClassFields, {}", (int)jvmti_err);
		env->DeleteLocalRef(klass);
		return klass_mappings;
	}

	LOGDEBUG("[MappingGenerator] Fields");
	for (int i = 0; i < field_count; i++)
	{
		char* name = nullptr;
		char* sig = nullptr;
		char* generic = nullptr;
		jint modifiers = 0;

		if (jvmti_err = jvmti_env->GetFieldName(klass, fields[i], &name, &sig, &generic); jvmti_err != JVMTI_ERROR_NONE)
		{
			LOGERROR("[MappingGenerator] Failed to call GetFieldName, {}", (int)jvmti_err);
			continue;
		}
		if (jvmti_err = jvmti_env->GetFieldModifiers(klass, fields[i], &modifiers); jvmti_err != JVMTI_ERROR_NONE)
		{
			LOGERROR("[MappingGenerator] Failed to call GetFieldModifiers, {}", (int)jvmti_err);
			continue;
		}

		const auto& field_mappings = mappings::fields;
		mappingFields mapping_field = mappingFields::NONE;
		for (auto& [field_mapping, field_name] : field_mappings)
		{
			if (strcmp(name, field_name.name.c_str()) == 0)
			{
				mapping_field = field_mapping;
				break;
			}
		}

		MappingField field;
		field.field = mapping_field;
		field.name = name;
		field.signature = sig;
		field.modifiers = modifiers;
		field.index = i;
		klass_mappings.fields.emplace_back(field);
	}

	LOGDEBUG("[MappingGenerator] Fields done");
	jvmti_env->Deallocate((unsigned char*)fields);

	LOGDEBUG("[MappingGenerator] Methods");

	jint method_count = 0;
	jmethodID* methods;
	jvmti_err = jvmti_env->GetClassMethods(klass, &method_count, &methods);
	if (jvmti_err != jvmtiError::JVMTI_ERROR_NONE)
	{
		LOGERROR("[MappingGenerator] Failed to call GetClassMethods, {}", (int)jvmti_err);
		env->DeleteLocalRef(klass);
		return klass_mappings;
	}
	std::vector<unsigned char> bytecodes_vec;

	for (int i = 0; i < method_count; i++)
	{
		jint bytecodes_count = 0;
		jint modifiers = 0;
		char* name = nullptr;
		char* sig = nullptr;
		char* generic = nullptr;
		unsigned char* bytecodes = nullptr;

		if (jvmti_err = jvmti_env->GetBytecodes(methods[i], &bytecodes_count, &bytecodes); jvmti_err != JVMTI_ERROR_NONE)
		{
			LOGERROR("[MappingGenerator] Failed to call GetBytecodes, {}", (int)jvmti_err);
			continue;
		}
		if (jvmti_err = jvmti_env->GetMethodModifiers(methods[i], &modifiers); jvmti_err != JVMTI_ERROR_NONE)
		{
			LOGERROR("[MappingGenerator] Failed to call GetMethodModifiers, {}", (int)jvmti_err);
			continue;
		}
		if (jvmti_err = jvmti_env->GetMethodName(methods[i], &name, &sig, &generic); jvmti_err != JVMTI_ERROR_NONE)
		{
			LOGERROR("[MappingGenerator] Failed to call GetMethodName, {}", (int)jvmti_err);
			continue;
		}

		bytecodes_vec.clear();
		bytecodes_vec.reserve(bytecodes_count);
		for (int j = 0; j < bytecodes_count; j++)
			bytecodes_vec.emplace_back(bytecodes[j]);

		const auto& method_mappings = mappings::methods;
		mapping mapping_method = mapping::NONE;
		for (auto& [method_mapping, method_name] : method_mappings)
		{
			if (strcmp(name, method_name.name.c_str()) == 0)
			{
				mapping_method = method_mapping;
				break;
			}
		}

		MethodMapping method;
		method.method = mapping_method;
		method.name = name;
		method.signature = sig;
		method.modifiers = modifiers;
		method.bytecodes = bytecodes_vec;
		method.index = i;
		klass_mappings.methods.emplace_back(method);
	}

	jvmti_env->Deallocate((unsigned char*)methods);
	env->DeleteLocalRef(klass);

	// get class index 

	jint class_count = 0;
	jclass* classes;
	jvmti_err = jvmti_env->GetLoadedClasses(&class_count, &classes);
	if (jvmti_err != JVMTI_ERROR_NONE)
	{
		LOGDEBUG("[MappingGenerator] GetLoadedClasses returned error {}", (int)jvmti_err);
		env->DeleteLocalRef(klass);
		return klass_mappings;
	}

	for (int i = 0; i < class_count; i++)
	{
		bool same_field_count = jvmfunc::oJVM_GetClassFieldsCount(env, classes[i]) == field_count;
		bool same_method_count = jvmfunc::oJVM_GetClassMethodsCount(env, classes[i]) == method_count;
		if (same_field_count && same_method_count)
		{
			class_index = i;
			break;
		}
	}

	jvmti_env->Deallocate((unsigned char*)classes);

	return klass_mappings;
}

void MappingGenerator::InitMappingsForClass(JNIEnv* env, jvmtiEnv* jvmti_env, jclass klass, const Mappings& mappings)
{
	jint klass_methods_count;
	jmethodID* methods = nullptr;
	jvmtiError err = jvmti_env->GetClassMethods(klass, &klass_methods_count, &methods);

	// save score of mapped methods (so we can check later for better ones)
	std::unordered_map<mapping, int> mapping_methods_scored;
	std::unordered_map<mappingFields, int> mapping_fields_scored;

	if (err == JVMTI_ERROR_NONE)
	{
		// match methods with minecraft methods

		for (int i = 0; i < klass_methods_count; i++)
		{
			jmethodID method = methods[i];
			char* name;
			char* sig;
			char* generic;
			err = jvmti_env->GetMethodName(method, &name, &sig, &generic);
			if (err != JVMTI_ERROR_NONE)
				continue;

			uint8_t* bytecodes;
			jint bytecode_count = 0;
			err = jvmti_env->GetBytecodes(method, &bytecode_count, &bytecodes);
			if (err != JVMTI_ERROR_NONE)
				continue;

			std::vector<uint8_t> current_bytecodes(bytecodes, bytecodes + bytecode_count);
			mapping max;
			max = mapping::NONE;
			float max_score = 0;

			for (const MethodMapping& m : mappings.methods)
			{
				if (m.method == mapping::NONE)
					continue;

				float similarity = math::jaccard_index(m.bytecodes, current_bytecodes);
				if (similarity > max_score)
				{
					max_score = similarity;
					max = m.method;

					//LOGDEBUG("{} {} = {}", name, m.name, max_score);
				}
			}

			if (max != mapping::NONE)
			{
				auto it = mapping_methods_scored.find(max);
				if (it != mapping_methods_scored.end())
				{
					if (max_score > it->second)
					{
						mappings::methods[max] = { name, sig };
						it->second = max_score;
					}
				}
				else
				{
					mappings::methods[max] = { name, sig };
					mapping_methods_scored.emplace(max, max_score);
				}
			}
		}

	}

	jint klass_fields_count;
	jfieldID* fields = nullptr;
	err = jvmti_env->GetClassFields(klass, &klass_fields_count, &fields);

	if (err == JVMTI_ERROR_NONE)
	{
		for (int i = 0; i < klass_fields_count; i++)
		{
			jfieldID field = fields[i];
			char* name;
			char* sig;
			char* generic;
			jint modifiers = -1;
			err = jvmti_env->GetFieldName(klass, field, &name, &sig, &generic);
			jvmti_env->GetFieldModifiers(klass, field, &modifiers);
			if (err != JVMTI_ERROR_NONE)
				continue;

			mappingFields max;
			max = mappingFields::NONE;
			// the lower the better the score 
			int min_score = std::numeric_limits<int>::max();

			for (const MappingField& f : mappings.fields)
			{
				if (f.field == mappingFields::NONE)
					continue;

				int score = 0;
				score += abs(f.index - i);
				if (f.modifiers != modifiers)
					score += 1;

				//LOGDEBUG("matching {} {}, score: {}", f.name, name, score);

				if (score < min_score)
				{
					min_score = score;
					max = f.field;
				}
			}

			if (max != mappingFields::NONE)
			{
				auto it = mapping_fields_scored.find(max);
				if (it != mapping_fields_scored.end())
				{
					if (min_score < it->second)
					{
						mappings::fields[max] = { name, sig };
						it->second = min_score;
					}
				}
				else
				{
					mappings::fields[max] = { name, sig };
					mapping_fields_scored.emplace(max, min_score);
				}
			}
		}
	}

	if (methods)
		jvmti_env->Deallocate((unsigned char*)methods);
	if (fields)
		jvmti_env->Deallocate((unsigned char*)fields);
}

}