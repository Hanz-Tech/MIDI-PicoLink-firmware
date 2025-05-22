import Ajv, { JSONSchemaType } from "ajv";

export interface Rp2040Config {
  command: "SAVEALL";
  filters: boolean[][];
  channels: boolean[];
}

const ajv = new Ajv();

const schema: JSONSchemaType<Rp2040Config> = {
  type: "object",
  properties: {
    command: { type: "string", const: "SAVEALL" },
    filters: {
      type: "array",
      minItems: 3,
      maxItems: 3,
      items: {
        type: "array",
        minItems: 8,
        maxItems: 8,
        items: { type: "boolean" }
      }
    },
    channels: {
      type: "array",
      minItems: 16,
      maxItems: 16,
      items: { type: "boolean" }
    }
  },
  required: ["command", "filters", "channels"],
  additionalProperties: false
};

export const validateConfig = ajv.compile(schema);
